#include "chunk/chunk_manager.hpp"
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include "core/timer.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

ChunkManager::ChunkManager(std::optional<siv::PerlinNoise::seed_type> seed)
    : shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
      waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY)
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

	if (seed.has_value()) {
		perlin = siv::PerlinNoise(seed.value());
		log::system_info("ChunkManager", "initialized with seed: {}", seed.value());
	} else {
		std::mt19937                       engine((std::random_device())());
		std::uniform_int_distribution<int> distribution(1, 999999);
		siv::PerlinNoise::seed_type        random_seed = distribution(engine);
		perlin                                         = siv::PerlinNoise(/*random_seed*/116896);
		log::system_info("ChunkManager", "initialized with random seed: {}", random_seed);
	}


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
	auto it = chunks.find(Chunk::worldToChunk(world_pos));
	if (it == chunks.end())
		return;
	chunks.erase(it);
}

bool ChunkManager::updateBlock(glm::vec3 world_pos, Block::blocks newType)
{
	Chunk* chunk = getChunk(world_pos);
	if (!chunk)
		return false;
	glm::ivec3 localPos = Chunk::worldToLocal(world_pos);

	chunk->setBlockAt(localPos.x, localPos.y, localPos.z, newType);
	chunk->updateMesh();
	return true;
}

float ChunkManager::LayeredPerlin(float x, float z, int octaves, float baseFreq, float baseAmp, float lacunarity, float persistence)
{
	float total     = 0.0f;
	float frequency = baseFreq;
	float amplitude = baseAmp;

	for (int i = 0; i < octaves; ++i) {
		total += (perlin.noise2D(x * frequency, z * frequency)) * amplitude; // Not the _01 variant, so it's in [-1, 1]
		frequency *= lacunarity;
		amplitude *= persistence;
	}
	return std::pow(std::abs(total), 1.3f) * glm::sign(total);
}
bool ChunkManager::neighborsAreGenerated(Chunk* chunk)
{
	/*
	return chunk &&
	       chunk->leftChunk.lock() && chunk->leftChunk.lock()->state >= ChunkState::Generated &&
	       chunk->rightChunk.lock() && chunk->rightChunk.lock()->state >= ChunkState::Generated &&
	       chunk->frontChunk.lock() && chunk->frontChunk.lock()->state >= ChunkState::Generated &&
	       chunk->backChunk.lock() && chunk->backChunk.lock()->state >= ChunkState::Generated;
		   */
	return false;
}
void ChunkManager::loadChunksAroundPos(const glm::ivec3& playerChunkPos, int renderDistance)
{

#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	if (renderDistance <= 0)
		return;
	Timer chunksTimer("chunk_generation");

	const int side = 2 * renderDistance + 1;

	glm::ivec2 region = {side * CHUNK_SIZE.x, side * CHUNK_SIZE.z};

	// Precomputed noise for the full region
	static std::vector<float> cachedNoiseRegion;
	static glm::ivec2         lastRegionSize  = {-1, -1};
	static glm::ivec3         lastNoiseOrigin = {-999999, 0, -999999};

	const glm::ivec3 noiseOrigin = {
	    (playerChunkPos.x - renderDistance) * CHUNK_SIZE.x,
	    0,
	    (playerChunkPos.z - renderDistance) * CHUNK_SIZE.z};

	{
		Timer noiseTimer("Noise precomputation");
		if (lastRegionSize != region || lastNoiseOrigin != noiseOrigin) {
			cachedNoiseRegion.resize(region.x * region.y);
			for (int y = 0; y < region.y; ++y) {
				for (int x = 0; x < region.x; ++x) {
					float wx                            = float((playerChunkPos.x - renderDistance) * CHUNK_SIZE.x + x);
					float wz                            = float((playerChunkPos.z - renderDistance) * CHUNK_SIZE.z + y);
					cachedNoiseRegion[y * region.x + x] = LayeredPerlin(wx, wz, 3, 0.003f, 1.2f);
				}
			}
			lastRegionSize  = region;
			lastNoiseOrigin = noiseOrigin;
		}
	}

	std::vector<float> noiseRegion = cachedNoiseRegion;

	for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
		for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
			glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};
			auto [it, inserted] = chunks.try_emplace(chunkPos, std::make_shared<Chunk>(chunkPos));
			if (inserted) {
				glm::vec3  worldPos = Chunk::chunkToWorld(chunkPos);

				int offsetX = worldPos.x - noiseOrigin.x;
				int offsetZ = worldPos.z - noiseOrigin.z;

				auto& chunk = it->second;
				if (chunk->state == ChunkState::Empty) {
					Timer terrain_timer("Chunk terrain generation");
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
	glm::ivec3 chunkPos = Chunk::worldToChunk(worldPos);
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
	auto       it       = chunks.find(Chunk::worldToChunk(worldPos));
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
	glm::ivec3 playerChunk{Chunk::worldToChunk(playerPos)};

	if (playerChunk.x != last_player_chunk_pos.x || playerChunk.z != last_player_chunk_pos.z) {
		if (renderDistance > 0)
			loadChunksAroundPos(playerChunk, renderDistance);
		if (renderDistance > 0)
			unloadDistantChunks(playerChunk, renderDistance + 3);

		last_player_chunk_pos.x = playerChunk.x;
		last_player_chunk_pos.z = playerChunk.z;
	}
}
