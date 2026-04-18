module;
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
#include <gl.h>
module chunk_manager;

import timer;
import logger;

ChunkManager::ChunkManager()
	: noise(92368123),
	shader2("Chunk2", SHADERS_DIRECTORY / "main.vs", SHADERS_DIRECTORY / "main.fs")
{
	DrawArraysIndirectCommand cmd{0, 1, 0, 0};
	// temp_indirect = ssbo::PersistentWrite(&DrawArraysIndirectCommand(0, 1, 0, 0), sizeof(DrawArraysIndirectCommand));

	mainThreadMeshData.opaqueMask = new uint64_t[CS_P2] { 0 };
	mainThreadMeshData.faceMasks = new uint64_t[CS_2 * 6] { 0 };
	mainThreadMeshData.forwardMerged = new uint8_t[CS_2] { 0 };
	mainThreadMeshData.rightMerged = new uint8_t[CS] { 0 };
	mainThreadMeshData.vertices = new std::vector<uint64_t>(100000);
	mainThreadMeshData.maxVertices = 100000;

	chunkRenderer.init();

	int size = 4;
	for (std::uint32_t x = 0; x < size; x++) {
		for (std::uint32_t z = 0; z < size; z++) {
      std::uint32_t y = 0; // flat Y for now

      std::memset(mainThreadMeshData.opaqueMask, 0, CS_P2 * sizeof(uint64_t));

			// Allocate voxel array for this chunk
			std::vector<std::uint8_t> voxels(CS_P3, 0);
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
      auto v = parse_xyz_key(get_xyz_key(x, y, z));
      glm::ivec3 chunkPos(v.x, v.y, v.z);
			// glm::ivec3 chunkPos = parse_xyz_key(get_xyz_key(x, y, z));
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
void ChunkManager::render_opaque(const Transform& ts, const FrustumVolume& fv) noexcept {
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
	// Chunk* chunk = getChunk(world_pos);
	// if (!chunk)
	// 	return false;
	// glm::ivec3 localPos = world_to_local(world_pos);
	//
	// chunk->set_block_type(localPos.x, localPos.y, localPos.z, newType);
	// if (!chunk->in_dirty_list) {
	// 	dirty_chunks.emplace_back(chunk);
	// 	chunk->in_dirty_list = true;
	// }
	// return true;
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
	// auto it = chunks.find(world_to_chunk(world_pos));
	// if (it != chunks.end())
	// 	return it->second.get();
	// else {
  {
#if defined(DEBUG)
		log::system_error("ChunkManager", "Chunk at glm::vec3({}, {}, {}) not found!", world_pos.x, world_pos.y, world_pos.z);
#endif
		return nullptr;
	}
}
void ChunkManager::update_mesh(Chunk *chunk) noexcept {


}

void ChunkManager::update_meshes() noexcept
{
	// if (dirty_chunks.empty()) return;
	//
	// face_flags.bind_base(2);
	// prefix.bind_base(5);
	// group_totals.bind_base(6);
	// temp_faces.bind_base(7);
	// for (auto& chunk : dirty_chunks) {
	// 	update_mesh(chunk);
	// 	chunk->in_dirty_list = false;
	// 	chunk->changed = false;
	// }
	// dirty_chunks.clear();
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
			}
		}
	}
}
void ChunkManager::unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance) noexcept
{

}
