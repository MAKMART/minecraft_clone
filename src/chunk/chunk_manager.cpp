#include "chunk/chunk_manager.hpp"
#include <cmath>
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include "core/timer.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

ChunkManager::ChunkManager(std::optional<int> seed)
    : shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
      waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY),
	  compute("Chunk_compute", SHADERS_DIRECTORY / "chunk.comp")
{

	log::info("Loading texture atlas from {} with working dir = {}", BLOCK_ATLAS_TEXTURE_DIRECTORY.string(), WORKING_DIRECTORY.string());
	log::info("Initializing Chunk Manager...");

	if (CHUNK_SIZE.x <= 0 || CHUNK_SIZE.y <= 0 || CHUNK_SIZE.z <= 0) {
		log::system_error("ChunkManager", "CHUNK_SIZE < 0");
		std::exit(1);
	}
	if ((CHUNK_SIZE.x & (CHUNK_SIZE.x - 1)) != 0 || (CHUNK_SIZE.y & (CHUNK_SIZE.y - 1)) != 0 || (CHUNK_SIZE.z & (CHUNK_SIZE.z - 1)) != 0) {
		log::system_error("ChunkManager", "CHUNK_SIZE must be a power of 2");
		std::exit(1);
	}
	perlin_node = FastNoise::New<FastNoise::Perlin>();
	fractal_node = FastNoise::New<FastNoise::FractalFBm>();
	fractal_node->SetSource(perlin_node);
	fractal_node->SetOctaveCount(4);
	fractal_node->SetLacunarity(4.48f);
	fractal_node->SetGain(0.440f);
	fractal_node->SetWeightedStrength(-0.32f);
	compute.setIVec3("CHUNK_SIZE", CHUNK_SIZE);
	glCreateVertexArrays(1, &VAO);
}
ChunkManager::~ChunkManager()
{
	clearChunks();
	if (VAO)
		glDeleteVertexArrays(1, &VAO);
}

void ChunkManager::unloadChunk(glm::vec3 world_pos)
{
	auto it = chunks.find(Chunk::world_to_chunk(world_pos));
	if (it == chunks.end())
		return;
	chunks.erase(it);
}

bool ChunkManager::updateBlock(glm::vec3 world_pos, Block::blocks newType)
{
	Chunk* chunk = getChunk(world_pos);
	if (!chunk)
		return false;
	glm::ivec3 localPos = Chunk::world_to_local(world_pos);

	chunk->setBlockAt(localPos.x, localPos.y, localPos.z, newType);
	compute.use();
	chunk->updateMesh();
	return true;
}

void ChunkManager::loadChunksAroundPos(const glm::ivec3& playerChunkPos, int renderDistance)
{

#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	chunks.reserve((2*renderDistance + 1)*(2*renderDistance + 1));

	if (renderDistance <= 0)
		return;
	Timer chunksTimer("chunk_generation");

	const int side = 2 * renderDistance + 1;

	glm::ivec2 region = {side * CHUNK_SIZE.x, side * CHUNK_SIZE.z};

	// Precomputed noise for the full region
	const glm::ivec3 noiseOrigin = {
	    (playerChunkPos.x - renderDistance) * CHUNK_SIZE.x,
	    0,
	    (playerChunkPos.z - renderDistance) * CHUNK_SIZE.z};

	{
		Timer noiseTimer("Noise precomputation");
		if (lastRegionSize != region || lastNoiseOrigin != noiseOrigin)
		{
			cachedNoiseRegion.resize(region.x * region.y);

			constexpr float step = 0.4f;

			// FastNoise2 fills row-major: [y * width + x]
			fractal_node->GenUniformGrid2D(
					cachedNoiseRegion.data(),
					static_cast<float>(noiseOrigin.x),
					static_cast<float>(noiseOrigin.z),
					region.x,
					region.y,
					step,
					step,
					SEED
					);

			lastRegionSize  = region;
			lastNoiseOrigin = noiseOrigin;
		}
	}


	auto& noiseRegion = cachedNoiseRegion;

	compute.use();
	for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
		for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
			glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};
			auto [it, inserted] = chunks.try_emplace(chunkPos, std::make_shared<Chunk>(chunkPos));
			if (inserted) {
				int offsetX = (dx + renderDistance) * CHUNK_SIZE.x;
				int offsetZ = (dz + renderDistance) * CHUNK_SIZE.z;

				auto& chunk = it->second;
				if (chunk->state == ChunkState::Empty) {
					//Timer terrain_timer("Chunk terrain generation");
					chunk->generate(std::span{noiseRegion}, region.x, offsetX, offsetZ);
					chunk->updateMesh();
					chunk->state = ChunkState::Generated;
				}
			}
		}
	}
}

void ChunkManager::unloadDistantChunks(const glm::ivec3& playerChunkPos, int unloadDistance)
{

	const int unloadDistSquared = unloadDistance * unloadDistance; // Avoid sqrt()

	std::erase_if(chunks, [&](const auto& pair) {
		glm::ivec3 chunkPos = pair.first;

		int distX       = chunkPos.x - playerChunkPos.x;
		int distZ       = chunkPos.z - playerChunkPos.z;
		int distSquared = distX * distX + distZ * distZ;

		return distSquared > unloadDistSquared; // Remove if too far
	});
}
std::shared_ptr<Chunk> ChunkManager::getOrCreateChunk(glm::vec3 worldPos)
{
	glm::ivec3 chunkPos = Chunk::world_to_chunk(worldPos);
	auto       it       = chunks.find(chunkPos);
	if (it != chunks.end() && it->second)
		return it->second;

	auto newChunk    = std::make_shared<Chunk>(chunkPos);
	chunks[chunkPos] = newChunk;
	return newChunk;
}

Chunk* ChunkManager::getChunk(glm::vec3 worldPos) const
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	auto it = chunks.find(Chunk::world_to_chunk(worldPos));
	if (it != chunks.end()) {
		return it->second.get(); // Return raw pointer (Chunk*)
	} else {
#if defined(DEBUG)
		log::system_error("ChunkManager", "Chunk at {} not found!", glm::to_string(worldPos));
#endif
		return nullptr;
	}
}
void ChunkManager::generateChunks(glm::vec3 playerPos, unsigned int renderDistance)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	glm::ivec3 playerChunk{Chunk::world_to_chunk(playerPos)};

	if (playerChunk.x != last_player_chunk_pos.x || playerChunk.z != last_player_chunk_pos.z) {
		if (renderDistance > 0)
			loadChunksAroundPos(playerChunk, renderDistance);
		if (renderDistance > 0)
			unloadDistantChunks(playerChunk, renderDistance + 3);

		last_player_chunk_pos.x = playerChunk.x;
		last_player_chunk_pos.z = playerChunk.z;
	}
}
