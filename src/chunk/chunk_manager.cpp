#include "chunk/chunk_manager.hpp"
#include <cmath>
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include "core/timer.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "graphics/shader.hpp"
#include <glm/gtx/norm.hpp>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
ChunkManager::ChunkManager(std::optional<int> seed)
    : shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
      waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY),
	  voxel_buffer("voxel_buffer", SHADERS_DIRECTORY / "voxel_buffer.comp"),
	  write_faces("write_faces", SHADERS_DIRECTORY / "write_faces.comp"),
	  add_global_offsets("add_global_offsets", SHADERS_DIRECTORY / "add_global_offsets.comp"),
	  prefix_sum("prefix_sum", SHADERS_DIRECTORY / "prefix_sum.comp"),
	  compute_global_offsets("compute_global_offsets", SHADERS_DIRECTORY / "compute_global_offsets.comp"),
	  indirect("indirect", SHADERS_DIRECTORY / "indirect_buffer_fill.comp"),
	  // Could be PersistentWrite
	  global_faces(SSBO::Dynamic(nullptr, MAX_FACES_BUFFER_BYTES)),  // or just Dynamic
	  global_indirect(SSBO::Dynamic(nullptr, 4096 * sizeof(DrawArraysIndirectCommand))),  // generous initial size
	  model_ssbo(SSBO::Dynamic(nullptr, model_ssbo_capacity))
{

	log::info("Loading texture atlas from {} with working dir = {}", BLOCK_ATLAS_TEXTURE_DIRECTORY.string(), WORKING_DIRECTORY.string());
	log::info("Initializing Chunk Manager...");

	if constexpr (CHUNK_SIZE.x <= 0 || CHUNK_SIZE.y <= 0 || CHUNK_SIZE.z <= 0) {
		log::system_error("ChunkManager", "CHUNK_SIZE < 0");
		std::exit(1);
	}
	if constexpr ((CHUNK_SIZE.x & (CHUNK_SIZE.x - 1)) != 0 || (CHUNK_SIZE.y & (CHUNK_SIZE.y - 1)) != 0 || (CHUNK_SIZE.z & (CHUNK_SIZE.z - 1)) != 0) {
		log::system_error("ChunkManager", "CHUNK_SIZE must be a power of 2");
		std::exit(1);
	}

	perlin_node = FastNoise::New<FastNoise::Perlin>();
	fractal_node = FastNoise::New<FastNoise::FractalFBm>();
	fractal_node->SetSource(perlin_node);
	fractal_node->SetOctaveCount(2);
	//fractal_node->SetLacunarity(4.48f);

	face_flags = SSBO::PersistentWrite(nullptr, flag_words * sizeof(uint));
	//normals = SSBO::Dynamic(nullptr, TOTAL_FACES * 4 * sizeof(uint));
	group_totals = SSBO::PersistentWrite(nullptr, num_workgroups * sizeof(uint));
	prefix = SSBO::PersistentWrite(nullptr, Chunk::TOTAL_FACES * sizeof(uint));

	voxel_buffer.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	prefix_sum.setUInt("total_faces", Chunk::TOTAL_FACES);
	write_faces.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	write_faces.setUInt("total_faces", Chunk::TOTAL_FACES);
	shader.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	compute_global_offsets.setUInt("num_groups", num_workgroups);
	add_global_offsets.setUInt("total_faces", Chunk::TOTAL_FACES);

	free_blocks.emplace_back(0, MAX_FACES_BUFFER_BYTES);

	opaqueChunks.reserve(chunks.size());
	transparentChunks.reserve(chunks.size());
	temp_faces = SSBO::PersistentWrite(nullptr, Chunk::TOTAL_FACES * sizeof(face_gpu));
	DrawArraysIndirectCommand cmd{0, 1, 0, 0};
	temp_indirect = SSBO::PersistentWrite(&cmd, sizeof(DrawArraysIndirectCommand));
}
ChunkManager::~ChunkManager() noexcept
{
	chunks.clear();
}
void ChunkManager::update_mesh(Chunk *chunk) noexcept 
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	if (!chunk->has_any_blocks()) {
		// Return existing memory if chunk becomes empty
		deallocate_faces(chunk->faces_offset, chunk->current_mesh_bytes);
		chunk->faces_offset = 0;
		chunk->current_mesh_bytes = 0;
		chunk->visible_face_count = 0;
		return;
	} else if (chunk->block_ssbo.id() && chunk->changed)
		chunk->block_ssbo.update_data(chunk->get_block_data(), sizeof(Block) * Chunk::SIZE);

	glClearNamedBufferData(face_flags.id(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

	chunk->block_ssbo.bind_to_slot(1);

	voxel_buffer.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	prefix_sum.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	compute_global_offsets.use();
	glDispatchCompute(num_scan_groups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	add_global_offsets.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	write_faces.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	temp_indirect.bind_to_slot(9);
	indirect.use();
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

	auto* cmd = temp_indirect.mapped<DrawArraysIndirectCommand>();
	uint32_t new_face_count = cmd->count / 6;

	// 1. Release old memory
	deallocate_faces(chunk->faces_offset, chunk->current_mesh_bytes);

	if (new_face_count == 0) {
		chunk->faces_offset = 0;
		chunk->current_mesh_bytes = 0;
		chunk->visible_face_count = 0;
		return;
	}
	// 2. Allocate new memory
	GLsizeiptr needed_bytes = static_cast<GLsizeiptr>(new_face_count) * sizeof(face_gpu);
	GLintptr new_offset = allocate_faces(needed_bytes);

	if (new_offset != -1) {
		chunk->faces_offset = new_offset;
		chunk->current_mesh_bytes = needed_bytes;
		chunk->visible_face_count = new_face_count;

		// 3. Copy from temp compute buffer to the global faces buffer
		face_gpu* face_ptr = temp_faces.mapped<face_gpu>();
		glCopyNamedBufferSubData(temp_faces.id(), global_faces.id(), 0, chunk->faces_offset, needed_bytes);
	}
}
GLintptr ChunkManager::allocate_faces(GLsizeiptr size) {
    if (size == 0) return 0;

    for (auto it = free_blocks.begin(); it != free_blocks.end(); ++it) {
        if (it->size >= size) {
            GLintptr offset = it->offset;
            if (it->size == size) {
                free_blocks.erase(it);
            } else {
                it->offset += size;
                it->size -= size;
            }
            return offset;
        }
    }
    log::error("ChunkManager", "GPU Face Buffer is full or too fragmented!");
    return -1; // Allocation failed
}

void ChunkManager::deallocate_faces(GLintptr offset, GLsizeiptr size) {
    if (size == 0) return;

    free_blocks.push_back({offset, size});

    // Sort blocks by offset to allow merging adjacent blocks
    std::sort(free_blocks.begin(), free_blocks.end(), [](const auto& a, const auto& b) {
        return a.offset < b.offset;
    });

    // Merge adjacent blocks
    for (size_t i = 0; i < free_blocks.size() - 1; ) {
        if (free_blocks[i].offset + free_blocks[i].size == free_blocks[i+1].offset) {
            free_blocks[i].size += free_blocks[i+1].size;
            free_blocks.erase(free_blocks.begin() + i + 1);
        } else {
            ++i;
        }
    }
}
void ChunkManager::update_meshes() noexcept
{
	if (dirty_chunks.empty()) return;
	
	face_flags.bind_to_slot(2);
	prefix.bind_to_slot(5);
	group_totals.bind_to_slot(6);
	temp_faces.bind_to_slot(7);
	for (auto& chunk : dirty_chunks) {
			update_mesh(chunk);
			chunk->in_dirty_list = false;
			chunk->changed = false;
	}
	dirty_chunks.clear();
}
bool ChunkManager::updateBlock(glm::vec3 world_pos, Block::blocks newType) noexcept
{
	Chunk* chunk = getChunk(world_pos);
	if (!chunk)
		return false;
	glm::ivec3 localPos = Chunk::world_to_local(world_pos);

	chunk->set_block_type(localPos.x, localPos.y, localPos.z, newType);
	if (!chunk->in_dirty_list) {
		dirty_chunks.emplace_back(chunk);
		chunk->in_dirty_list = true;
	}
	return true;
}
Chunk* ChunkManager::getChunk(glm::vec3 world_pos) const noexcept
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	auto it = chunks.find(Chunk::world_to_chunk(world_pos));
	if (it != chunks.end())
		return it->second.get();
	else {
#if defined(DEBUG)
		log::system_error("ChunkManager", "Chunk at {} not found!", glm::to_string(world_pos));
#endif
		return nullptr;
	}
}
void ChunkManager::load_around_pos(glm::ivec3 playerChunkPos, unsigned int renderDistance) noexcept
{
#if defined(TRACY_ENABLE)
    ZoneScoped;
#endif

    if (renderDistance == 0)
        return;
	Timer chunksTimer("chunk_generation");

	const int dist = static_cast<int>(renderDistance);
	const int max_dist_square = dist * dist;

	for (int dz = -dist; dz <= dist; ++dz)
	{
		for (int dy = -dist; dy <= dist; ++dy) 
		{
			for (int dx = -dist; dx <= dist; ++dx)
			{
				if (dx*dx + dy*dy + dz*dz > max_dist_square) continue;
				glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(dx, dy, dz);

				auto [it, inserted] = chunks.try_emplace(chunkPos, nullptr);

				if (!inserted)
					continue;

				// If we reach here, it's a new chunk
				it->second = std::make_unique<Chunk>(chunkPos);
				it->second->generate(fractal_node, SEED);

				if (it->second->has_any_blocks()) {
					dirty_chunks.emplace_back(it->second.get());
					it->second->in_dirty_list = true;
				}
			}
		}
	}
}
void ChunkManager::unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance) noexcept
{
    const unsigned int unloadDistSquared = unloadDistance * unloadDistance;

    // Only iterate over chunks near the edges
    for (auto it = chunks.begin(); it != chunks.end();)
    {
        glm::vec3 chunkPos = it->first;
		glm::vec3 distance = chunkPos - glm::vec3(playerChunkPos);

        if ((unsigned int)glm::length2(distance) > unloadDistSquared) {
			deallocate_faces(it->second->faces_offset, it->second->current_mesh_bytes);
            it = chunks.erase(it);
		} else
            ++it;
    }
}

void ChunkManager::generate_chunks(glm::vec3 playerPos, unsigned int renderDistance) noexcept
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	glm::ivec3 playerChunk{Chunk::world_to_chunk(playerPos)};

	if (playerChunk != last_player_chunk_pos) {
		load_around_pos(playerChunk, renderDistance);
		unload_around_pos(playerChunk, renderDistance + 1);
		last_player_chunk_pos = playerChunk;
	}
	update_meshes();
}
