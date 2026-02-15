#include "chunk/chunk_manager.hpp"
#include <cmath>
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include "core/timer.hpp"
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
	  indirect("indirect", SHADERS_DIRECTORY / "indirect_buffer_fill.comp")
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

	constexpr uint num_workgroups = (Chunk::TOTAL_FACES + 255u) / 256u;
	constexpr uint num_scan_groups = (num_workgroups + 255u) / 256u;

	const constexpr uint flag_words = (Chunk::TOTAL_FACES + 31u) / 32u;
	face_flags   = SSBO::Dynamic(nullptr, flag_words * sizeof(uint));
	//normals = SSBO::Dynamic(nullptr, TOTAL_FACES * 4 * sizeof(uint));
	group_totals  = SSBO::Dynamic(nullptr, num_workgroups * sizeof(uint));
	prefix       = SSBO::Dynamic(nullptr, Chunk::TOTAL_FACES * sizeof(uint));

	voxel_buffer.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	prefix_sum.setUInt("total_faces", Chunk::TOTAL_FACES);
	write_faces.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	write_faces.setUInt("total_faces", Chunk::TOTAL_FACES);
	shader.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	compute_global_offsets.setUInt("num_groups", num_workgroups);
	add_global_offsets.setUInt("total_faces", Chunk::TOTAL_FACES);
	glCreateVertexArrays(1, &VAO);
}
ChunkManager::~ChunkManager()
{
	chunks.clear();
	if (VAO)
		glDeleteVertexArrays(1, &VAO);
}

void ChunkManager::update_mesh(Chunk *chunk) noexcept 
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	if (chunk->block_ssbo.id())
		chunk->block_ssbo.update_data(chunk->get_block_data(), sizeof(Block) * Chunk::SIZE);


	glClearNamedBufferData(
			face_flags.id(),
			GL_R32UI,
			GL_RED_INTEGER,
			GL_UNSIGNED_INT,
			nullptr
			);

	chunk->block_ssbo.bind_to_slot(1);
	face_flags.bind_to_slot(2);
	prefix.bind_to_slot(5);
	group_totals.bind_to_slot(6);
	chunk->faces.bind_to_slot(7);


	uint num_workgroups = (Chunk::TOTAL_FACES + 255u) / 256u;
	uint num_scan_groups = (num_workgroups + 255u) / 256u;

	voxel_buffer.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	prefix_sum.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	compute_global_offsets.use();
	glDispatchCompute(num_scan_groups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	/* Uncomment this and comment out the compute_global_offsets pass if you want to do the exclusive prefix sum on the CPU (This will copy the buffer from the GPU to the CPU which isn't ideal)
	// === READ WORKGROUP TOTALS (tiny buffer) ===
	std::vector<std::uint32_t> wg_totals(num_workgroups);
	glGetNamedBufferSubData(chunk->group_totals.id(), 0,
			num_workgroups * sizeof(uint32_t), wg_totals.data());

	// === EXCLUSIVE PREFIX SUM ON CPU ===
	uint32_t running_sum = 0;
	for (uint32_t i = 0; i < num_workgroups; ++i) {
		uint32_t old = wg_totals[i];
		wg_totals[i] = running_sum;          // exclusive base for this group
		running_sum += old;
	}

	// === WRITE BACK ===
	glNamedBufferSubData(chunk->group_totals.id(), 0,
			num_workgroups * sizeof(uint32_t), wg_totals.data());
	//chunk->group_totals.update_data(wg_totals.data(), num_workgroups * sizeof(uint32_t));
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	*/


	add_global_offsets.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	write_faces.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	chunk->indirect_ssbo.bind_to_slot(9);
	indirect.use();
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
}
void ChunkManager::update_meshes() noexcept
{
	for (auto chunk : dirty_chunks) {
		update_mesh(chunk);
		chunk->in_dirty_list = false;
	}

}
bool ChunkManager::updateBlock(glm::vec3 world_pos, Block::blocks newType)
{
	Chunk* chunk = getChunk(world_pos);
	if (!chunk)
		return false;
	glm::ivec3 localPos = Chunk::world_to_local(world_pos);

	chunk->set_block_type(localPos.x, localPos.y, localPos.z, newType);
	if (!chunk->in_dirty_list)
		dirty_chunks.emplace_back(chunk);
	return true;
}
Chunk* ChunkManager::getChunk(glm::vec3 world_pos) const
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
void ChunkManager::load_around_pos(glm::ivec3 playerChunkPos, unsigned int renderDistance)
{
#if defined(TRACY_ENABLE)
    ZoneScoped;
#endif

    if (renderDistance == 0)
        return;
	Timer chunksTimer("chunk_generation");

    const int side = 2 * renderDistance + 1;
	const int dist = static_cast<int>(renderDistance);

	for (int dz = -dist; dz <= dist; ++dz)
	{
		for (int dy = -dist; dy <= dist; ++dy) 
		{
			for (int dx = -dist; dx <= dist; ++dx)
			{
				if (dx*dx + dy*dy + dz*dz > renderDistance*renderDistance) continue;
				glm::ivec3 chunkPos{playerChunkPos.x + dx, playerChunkPos.y + dy, playerChunkPos.z + dz};

				if (chunks.find(chunkPos) != chunks.end())
					continue;

				auto& chunk = chunks[chunkPos] = std::make_unique<Chunk>(chunkPos);

				int offsetX = (dx + renderDistance) * CHUNK_SIZE.x;
				int offsetY = (dy + renderDistance) * CHUNK_SIZE.y;
				int offsetZ = (dz + renderDistance) * CHUNK_SIZE.z;

				chunk->generate(fractal_node, SEED);

				if (chunk->has_any_blocks() && !chunk->in_dirty_list) {
					dirty_chunks.emplace_back(chunk.get());
					chunk->in_dirty_list = true;
				}
			}
		}
	}
}
void ChunkManager::unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance)
{
    const int unloadDistSquared = unloadDistance * unloadDistance;

    // Only iterate over chunks near the edges
    for (auto it = chunks.begin(); it != chunks.end(); )
    {
        glm::vec3 chunkPos = it->first;
		glm::vec3 distance = chunkPos - glm::vec3(playerChunkPos);

        if (glm::length2(distance) > unloadDistSquared)
            it = chunks.erase(it); // Unload chunk
        else
            ++it;
    }
}

void ChunkManager::generate_chunks(glm::vec3 playerPos, unsigned int renderDistance)
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
	dirty_chunks.clear();
}
