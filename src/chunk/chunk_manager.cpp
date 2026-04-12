module;
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
#include <glad/glad.h>   // Solves GL_TRIANGLES, GLuint, etc.
module chunk_manager;

import timer;
import logger;

ChunkManager::ChunkManager()
	: noise(92368123),
	shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
	shader2("Chunk2", SHADERS_DIRECTORY / "main.vs", SHADERS_DIRECTORY / "main.fs"),
	waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY),
	voxel_buffer("voxel_buffer", SHADERS_DIRECTORY / "voxel_buffer.comp"),
	write_faces("write_faces", SHADERS_DIRECTORY / "write_faces.comp"),
	merged_offsets("merged_offsets", SHADERS_DIRECTORY / "merged_offsets.comp"),
	prefix_sum("prefix_sum", SHADERS_DIRECTORY / "prefix_sum.comp"),
	indirect("indirect", SHADERS_DIRECTORY / "indirect_buffer_fill.comp"),
	// Could be PersistentWrite
	global_faces(dynamic_ssbo(MAX_FACES_BUFFER_BYTES)),  // or just Dynamic
	global_indirect(dynamic_ssbo(4096 * sizeof(DrawArraysIndirectCommand))),  // generous initial size
	prefix(persistent_ssbo(TOTAL_FACES * sizeof(face_gpu))),
	face_flags(persistent_ssbo(flag_words * sizeof(face_gpu))),
	group_totals(persistent_ssbo(num_workgroups * sizeof(face_gpu))),
	offset_ssbo(dynamic_ssbo(offset_ssbo_capacity)),
	temp_faces(persistent_ssbo(TOTAL_FACES * sizeof(face_gpu)))
{

	shader.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	voxel_buffer.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
	prefix_sum.setUInt("total_faces", TOTAL_FACES);
	write_faces.setUInt("total_faces", TOTAL_FACES);
	merged_offsets.setUInt("total_faces", TOTAL_FACES);
	merged_offsets.setUInt("num_groups", num_workgroups);

	free_blocks.emplace_back(0, MAX_FACES_BUFFER_BYTES);
	opaqueChunks.reserve(chunks.size());
	transparentChunks.reserve(chunks.size());
	DrawArraysIndirectCommand cmd{0, 1, 0, 0};
	// temp_indirect = ssbo::PersistentWrite(&DrawArraysIndirectCommand(0, 1, 0, 0), sizeof(DrawArraysIndirectCommand));

	mainThreadMeshData.opaqueMask = new uint64_t[CS_P2] { 0 };
	mainThreadMeshData.faceMasks = new uint64_t[CS_2 * 6] { 0 };
	mainThreadMeshData.forwardMerged = new uint8_t[CS_2] { 0 };
	mainThreadMeshData.rightMerged = new uint8_t[CS] { 0 };
	mainThreadMeshData.vertices = new std::vector<uint64_t>(10000);
	mainThreadMeshData.maxVertices = 10000;

	chunkRenderer.init();

	int size = 4;
	for (std::uint32_t x = 0; x < size; x++) {
		for (std::uint32_t z = 0; z < size; z++) {
      std::uint32_t y = 0; // flat Y for now

      std::memset(mainThreadMeshData.opaqueMask, 0, CS_P2 * sizeof(uint64_t));

			// Allocate voxel array for this chunk
			std::vector<uint8_t> voxels(CS_P3, 0);
			// Fill terrain
			// noise_2.generateTerrainV2(voxels.data(), mainThreadMeshData.opaqueMask, x, z, 30);
			// ────────────────────────────────────────────────
			//   1. World-space starting coordinate
			// ────────────────────────────────────────────────
			float worldStartX = x * float(CS);     // ← important: CS, not CS_P
			float worldStartZ = z * float(CS);

			std::vector<float> noise_data(CS_P3, 0);
			noise_2.gen_uniform_3d(noise_data, worldStartX, 0.0f, worldStartZ, CS_P, 1.0f, 30);

			// Optional: scale amplitude / remap to more useful range
			// You can multiply or remap here if needed
			// for (auto& v : noiseData) v = (v + 1.0f) * 0.5f;   // → [0..1]

			// ────────────────────────────────────────────────
			//   3. Voxel filling logic (same as before)
			// ────────────────────────────────────────────────
			for (int lx = 0; lx < CS_P; lx++) {
				for (int ly = CS_P - 1; ly >= 0; ly--) {     // ← better to go top→bottom
					for (int lz = 0; lz < CS_P; lz++) {
						int idx = get_zxy_index(lx, ly, lz);
						float heightNoise = noise_data[idx];

						// Simple height-based threshold
						// You can make this more sophisticated later
						bool shouldBeSolid = heightNoise > (float(ly) / float(CS_P));

						if (shouldBeSolid) {
							int idx_above = get_zxy_index(lx, ly + 1, lz);
							mainThreadMeshData.opaqueMask[(ly * CS_P) + lx] |= (1ULL << lz);

							// Simple surface / subsurface logic
							if (voxels[idx_above] == 0) {
								voxels[idx] = 3;           // grass / surface
							} else {
								voxels[idx] = 2;           // dirt / stone
							}
						}
						// Fill bedrock / deep stone below sea level
						else if (ly < 25) {
							mainThreadMeshData.opaqueMask[(ly * CS_P) + lx] |= (1ULL << lz);
							voxels[idx] = 1;               // stone
						}
					}
				}
			}

			// Mesh the chunk immediately
			mesh(voxels.data(), mainThreadMeshData);

			// Create draw commands
			glm::ivec3 chunkPos = parse_xyz_key(get_xyz_key(x, y, z));
			std::vector<DrawElementsIndirectCommand> commands(6);
			for (int i = 0; i < 6; i++) {
				if (mainThreadMeshData.faceVertexLength[i]) {
          std::int32_t baseInstance = (i << 24) | (chunkPos.z << 16) | (chunkPos.y << 8) | chunkPos.x;
					auto drawCommand = chunkRenderer.getDrawCommand(mainThreadMeshData.faceVertexLength[i], baseInstance);
					commands[i] = drawCommand;
					chunkRenderer.buffer(drawCommand, mainThreadMeshData.vertices->data() + mainThreadMeshData.faceVertexBegin[i]);
				}
			}

			chunkRenderData.push_back({ chunkPos, commands });
		}
	}


}
void ChunkManager::build_lists(const Transform& ts, const FrustumVolume& fv) noexcept 
{
	std::vector<DrawArraysIndirectCommand> commands;		
	opaqueChunks.clear();
	transparentChunks.clear();
	offset_data.clear();

	for (const auto& [pos, chunk_ptr] : chunks) {
		Chunk* c = chunk_ptr.get();
		if (c->visible_face_count > 0 && isAABBInsideFrustum(c->aabb, fv)) {
			//if (chunk->isTransparent())
			//transparentChunks.emplace_back(chunk.get());
			//else
			opaqueChunks.emplace_back(c);

			commands.push_back({
					.count = c->visible_face_count * 6,
					.instanceCount = 1,
					.first = (GLuint)(c->faces_offset / sizeof(face_gpu)) * 6,  // vertex offset in faces buffer
					.baseInstance = (GLuint)commands.size()
					});
			offset_data.emplace_back(glm::vec4{chunk_to_world(c->position), 1.0f});

		}
	}

	std::size_t needed_offset = offset_data.size() * sizeof(glm::vec4);
	if (needed_offset > offset_ssbo_capacity) {
		offset_ssbo.resize(needed_offset * 2);
		offset_ssbo_capacity = offset_ssbo.size();
	}
	offset_ssbo.upload(offset_data.data(), offset_data.size() * sizeof(glm::vec4));
	global_indirect.upload(commands.data(), commands.size() * sizeof(DrawArraysIndirectCommand));
	if (!transparentChunks.empty()) {
		std::sort(transparentChunks.begin(), transparentChunks.end(),
				[&](const auto& a, const auto& b) {
				glm::vec3 deltaA = ts.pos - a->aabb.center();
				float distA2 = deltaA.x*deltaA.x + deltaA.y*deltaA.y + deltaA.z*deltaA.z;
				glm::vec3 deltaB = ts.pos - b->aabb.center();
				float distB2 = deltaB.x*deltaB.x + deltaB.y*deltaB.y + deltaB.z*deltaB.z;
				return distA2 > distB2; // farthest first
				});

	}
}

void ChunkManager::render_opaque(const Transform& ts, const FrustumVolume& fv) noexcept {
	/*
	build_lists(ts, fv);  // Rebuild indirect every frame (or only if dirty/camera moved)

	if (opaqueChunks.empty()) return;

	shader.use();
	global_faces.bind_to_slot(7);
	offset_ssbo.bind_to_slot(10);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, global_indirect.id());

	MultiDrawArraysIndirectWrapper(GL_TRIANGLES, 0, opaqueChunks.size(), 0);  // stride=0 for tight pack
	*/
	glm::ivec3 cameraChunkPos = glm::floor(ts.pos / glm::vec3(CS));
	for (const auto& data : chunkRenderData) {
		for (int i = 0; i < 6; i++) {
			auto& d = data.faceDrawCommands[i];
			if (d.indexCount > 0) {
				chunkRenderer.addDrawCommand(d);
				switch (i) {
					case 0:
						if (cameraChunkPos.y >= data.chunkPos.y) {
							chunkRenderer.addDrawCommand(d);
						}
						break;

					case 1:
						if (cameraChunkPos.y <= data.chunkPos.y) {
							chunkRenderer.addDrawCommand(d);
						}
						break;

					case 2:
						if (cameraChunkPos.x >= data.chunkPos.x) {
							chunkRenderer.addDrawCommand(d);
						}
						break;

					case 3:
						if (cameraChunkPos.x <= data.chunkPos.x) {
							chunkRenderer.addDrawCommand(d);
						}
						break;

					case 4:
						if (cameraChunkPos.z >= data.chunkPos.z) {
							chunkRenderer.addDrawCommand(d);
						}
						break;

					case 5:
						if (cameraChunkPos.z <= data.chunkPos.z) {
							chunkRenderer.addDrawCommand(d);
						}
						break;
				}
			}
		}
	}

	// shader2.use();
	chunkRenderer.render();
}
bool ChunkManager::update_block(glm::ivec3 world_pos, Block::blocks newType) noexcept
{
	Chunk* chunk = getChunk(world_pos);
	if (!chunk)
		return false;
	glm::ivec3 localPos = world_to_local(world_pos);

	chunk->set_block_type(localPos.x, localPos.y, localPos.z, newType);
	if (!chunk->in_dirty_list) {
		dirty_chunks.emplace_back(chunk);
		chunk->in_dirty_list = true;
	}
	return true;
}

void ChunkManager::generate_chunks(glm::vec3 playerPos, unsigned int renderDistance) noexcept
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	glm::ivec3 playerChunk{world_to_chunk(playerPos)};

	if (playerChunk != last_player_chunk_pos) {
		load_around_pos(playerChunk, renderDistance);
		unload_around_pos(playerChunk, renderDistance + 1);
		last_player_chunk_pos = playerChunk;
	}
	update_meshes();
}
Chunk* ChunkManager::getChunk(glm::ivec3 world_pos) const noexcept
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	auto it = chunks.find(world_to_chunk(world_pos));
	if (it != chunks.end())
		return it->second.get();
	else {
#if defined(DEBUG)
		log::system_error("ChunkManager", "Chunk at glm::vec3({}, {}, {}) not found!", world_pos.x, world_pos.y, world_pos.z);
#endif
		return nullptr;
	}
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
	} else if (chunk->changed)
		// chunk->block_ssbo.update(chunk->block_types, sizeof(std::uint8_t) * SIZE);
		std::memcpy(chunk->block_ssbo.data<std::uint8_t>(), chunk->block_types, SIZE);

	glClearNamedBufferData(face_flags.id(), GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);

	chunk->block_ssbo.bind_base(1);

	voxel_buffer.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	prefix_sum.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	merged_offsets.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	write_faces.use();
	glDispatchCompute(num_workgroups, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// temp_indirect.bind_to_slot(9);
	indirect.use();
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

	// auto* cmd = temp_indirect.mapped<DrawArraysIndirectCommand>();
	// if (!cmd) log::system_error("Chunk_Manager", "temp_indirect.mapped<DrawArraysIndirectCommand>() returned nullptr");
	// std::uint32_t new_face_count = cmd->count / 6;
	std::uint32_t new_face_count = 6;

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
		face_gpu* face_ptr = temp_faces.data<face_gpu>();
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

    free_blocks.emplace_back(offset, size);

    // Sort blocks by offset to allow merging adjacent blocks
    std::sort(free_blocks.begin(), free_blocks.end(), [](const auto& a, const auto& b) {
        return a.offset < b.offset;
    });

    // Merge adjacent blocks
    for (std::size_t i = 0; i < free_blocks.size() - 1; ) {
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

	face_flags.bind_base(2);
	prefix.bind_base(5);
	group_totals.bind_base(6);
	temp_faces.bind_base(7);
	for (auto& chunk : dirty_chunks) {
		update_mesh(chunk);
		chunk->in_dirty_list = false;
		chunk->changed = false;
	}
	dirty_chunks.clear();
}
void ChunkManager::load_around_pos(glm::ivec3 playerChunkPos, unsigned int renderDistance) noexcept
{
#if defined(TRACY_ENABLE)
    ZoneScoped;
#endif

    if (renderDistance == 0) return;

	Timer chunksTimer("chunk_generation");

	const int dist = static_cast<int>(renderDistance);
	const int max_dist_square = dist * dist;

	// ─────────────────────────────────────────────────────────────
	//               TUNABLE TERRAIN PARAMETERS
	// ─────────────────────────────────────────────────────────────

															   // Height range & vertical distribution
	constexpr float    BASE_HEIGHT           = 14.0f;      // Approximate "sea level" / reference height
	constexpr float	   HEIGHT_AMPLITUDE     = 60.0f;
	constexpr float    MAX_TERRAIN_HEIGHT    = 100;        // Max possible mountain peak height (world space)
	constexpr float    MIN_TERRAIN_HEIGHT    = -20.0f;    // Lowest possible terrain floor (world space)

	// Noise → height mapping
	constexpr float    NOISE_TO_HEIGHT_SCALE = static_cast<float>(MAX_TERRAIN_HEIGHT - MIN_TERRAIN_HEIGHT);
	constexpr float    NOISE_VERTICAL_OFFSET = MIN_TERRAIN_HEIGHT;  // Shifts the [-1..1] → desired range

	// Surface layering
	constexpr int      DIRT_DEPTH            = 3;          // How many layers of dirt under grass
	constexpr int      BEACH_DEPTH           = 1;          // (currently unused — can be used for sand/gravel later)

	// ─────────────────────────────────────────────────────────────


	// FIX 1: Use a CONSTANT range for normalization, NOT min_max from the generator
	// For most FBM/Perlin, the raw range is approximately [-1.0, 1.0]
	constexpr float GLOBAL_MIN = -1.0f;
	constexpr float GLOBAL_MAX =  1.0f;
	constexpr float GLOBAL_RANGE = GLOBAL_MAX - GLOBAL_MIN;



	for (int dz = -dist; dz <= dist; ++dz)
	{
		for (int dy = -dist; dy <= dist; ++dy) 
		{
			for (int dx = -dist; dx <= dist; ++dx)
			{
				if (dx*dx + dy*dy + dz*dz > max_dist_square) continue;
				glm::ivec3 chunkPos = playerChunkPos + glm::ivec3(dx, dy, dz);

				/*
				uint64_t key = get_xyz_key(chunkPos.x, chunkPos.y, chunkPos.z);
				if (loaded_chunks.find(key) == loaded_chunks.end()) {
					loaded_chunks[key] = generate_chunk(chunkPos);
				}
				*/

				/*
				auto [it, inserted] = chunks.try_emplace(chunkPos, nullptr);

				if (!inserted)
					continue;

				// If we reach here, it's a new chunk
				it->second = std::make_unique<Chunk>(chunkPos);
				it->second->non_air_count = 0;
				constexpr int dirtDepth  = 3;
				constexpr int beachDepth = 1; // How wide the beach is vertically
				constexpr int seaLevel = 5;

				// 2. Calculate world origin for this chunk
				glm::vec3 chunk_world_origin = chunk_to_world(it->second->position);

				// 3. Generate noise for this specific chunk's area in world space
				auto min_max = noise.gen_grid_2d({chunk_world_origin.x, chunk_world_origin.z});

				float safe_min = std::max(min_max.min, -1.1f);
				float safe_max = std::min(min_max.max,  1.1f);
				float range    = safe_max - safe_min;
				float inv_range = (range > 1e-6f) ? 1.0f / range : 1.0f;

				// FIX 2: Outer loop must be LZ to match Row-Major memory layout [lz * width + lx]
				for (int lz = 0; lz < CHUNK_SIZE.z; ++lz)
				{
					for (int lx = 0; lx < CHUNK_SIZE.x; ++lx)
					{
						// Access noise sequentially
						float raw = noise.get_noise()[lz * CHUNK_SIZE.x + lx];

						// Normalize using the fixed global range
						float normalized = std::clamp((raw - GLOBAL_MIN) / GLOBAL_RANGE, 0.0f, 1.0f);

						float world_height_f = BASE_HEIGHT + (normalized - 0.5f) * HEIGHT_AMPLITUDE;
						world_height_f = std::clamp(world_height_f, MIN_TERRAIN_HEIGHT, MAX_TERRAIN_HEIGHT);

						int world_height = static_cast<int>(std::round(world_height_f));
						int local_surface_y = world_height - static_cast<int>(chunk_world_origin.y);

						// Block placement loop
						for (int ly = 0; ly < CHUNK_SIZE.y; ++ly)
						{
							int idx = it->second->get_index(lx, ly, lz);
							Block::blocks type = Block::blocks::AIR;

							if (ly <= local_surface_y)
							{
								if (ly == local_surface_y) type = Block::blocks::GRASS;
								else if (ly >= local_surface_y - DIRT_DEPTH) type = Block::blocks::DIRT;
								else type = Block::blocks::STONE;
								++it->second->non_air_count;
							}
							it->second->block_types[idx] = static_cast<std::uint8_t>(type);
						}
					}
				}

				it->second->changed = true;


				if (it->second->has_any_blocks()) {
					dirty_chunks.emplace_back(it->second.get());
					it->second->in_dirty_list = true;
				}
				*/
			}
		}
	}
}
void ChunkManager::unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance) noexcept
{
	/*
    const unsigned int unloadDistSquared = unloadDistance * unloadDistance;

    // Only iterate over chunks near the edges
    for (auto it = chunks.begin(); it != chunks.end();)
    {
		glm::ivec3 delta = it->first - playerChunkPos;
		int distSquared = delta.x*delta.x + delta.y*delta.y + delta.z*delta.z;
        if (distSquared > unloadDistSquared) {
			deallocate_faces(it->second->faces_offset, it->second->current_mesh_bytes);
            it = chunks.erase(it);
		} else
            ++it;
    }
	*/
	/*
	std::vector<uint64_t> chunksToRemove;
	for (auto& [key, chunk] : loaded_chunks) {
		if (glm::distance(glm::vec3(chunk.chunkPos), glm::vec3(playerChunkPos)) > unloadDistance) {
			for (auto& cmd : chunk.faceDrawCommands) {
				if (cmd) {
					chunkRenderer.removeDrawCommand(cmd);
					delete cmd;
				}
			}
			chunksToRemove.push_back(key);
		}
	}

	for (auto key : chunksToRemove) {
		loaded_chunks.erase(key);
	}
	*/

}
