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
	if (VAO) {
		glDeleteBuffers(1, &VAO);
	
	}

}

void ChunkManager::unloadChunk(glm::vec3 worldPos)
{
	auto it = chunks.find(Chunk::worldToChunk(worldPos));
	if (it == chunks.end())
		return;

	// Clear neighbor references in adjacent chunks
	auto chunk = it->second;
	if (auto left = chunk->leftChunk.lock()) {
		left->rightChunk.reset();
		left->updateMesh();
	}
	if (auto right = chunk->rightChunk.lock()) {
		right->leftChunk.reset();
		right->updateMesh();
	}
	if (auto front = chunk->frontChunk.lock()) {
		front->backChunk.reset();
		front->updateMesh();
	}
	if (auto back = chunk->backChunk.lock()) {
		back->frontChunk.reset();
		back->updateMesh();
	}

	chunks.erase(it);
}

void ChunkManager::updateBlock(glm::vec3 worldPos, Block::blocks newType)
{
	Chunk* chunk = getChunk(worldPos);

	if (!chunk)
		return;

	glm::ivec3 localPos = Chunk::worldToLocal(worldPos);

	chunk->setBlockAt(localPos.x, localPos.y, localPos.z, newType);
	chunk->updateMesh();

	// Update neighbors if the block is on a boundary
	if (localPos.x == 0) {
		if (auto neighbor = chunk->leftChunk.lock()) {
			neighbor->updateMesh();
		}
	}
	if (localPos.x == CHUNK_SIZE.x - 1) {
		if (auto neighbor = chunk->rightChunk.lock()) {
			neighbor->updateMesh();
		}
	}
	if (localPos.z == 0) {
		if (auto neighbor = chunk->backChunk.lock()) {
			neighbor->updateMesh();
		}
	}
	if (localPos.z == CHUNK_SIZE.z - 1) {
		if (auto neighbor = chunk->frontChunk.lock()) {
			neighbor->updateMesh();
		}
	}
	// Add y-boundary checks if needed
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

	return total; // raw value, unnormalized
}
bool ChunkManager::neighborsAreGenerated(Chunk* chunk)
{
	return chunk &&
	       chunk->leftChunk.lock() && chunk->leftChunk.lock()->state >= ChunkState::Generated &&
	       chunk->rightChunk.lock() && chunk->rightChunk.lock()->state >= ChunkState::Generated &&
	       chunk->frontChunk.lock() && chunk->frontChunk.lock()->state >= ChunkState::Generated &&
	       chunk->backChunk.lock() && chunk->backChunk.lock()->state >= ChunkState::Generated;
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

	// Pass 1: generate the noise map for all the chunks in the current region that we want to render
	{
		Timer noiseTimer("Noise precomputation");
		if (lastRegionSize != region || lastNoiseOrigin != noiseOrigin) {
			cachedNoiseRegion.resize(region.x * region.y);
			for (int y = 0; y < region.y; ++y) {
				for (int x = 0; x < region.x; ++x) {
					float wx                            = float((playerChunkPos.x - renderDistance) * CHUNK_SIZE.x + x);
					float wz                            = float((playerChunkPos.z - renderDistance) * CHUNK_SIZE.z + y);
					float rawNoise                      = LayeredPerlin(wx, wz, 7, 0.003f, 1.2f);
					cachedNoiseRegion[y * region.x + x] = std::pow(std::abs(rawNoise), 1.3f) * glm::sign(rawNoise);
				}
			}
			lastRegionSize  = region;
			lastNoiseOrigin = noiseOrigin;
		}
	}

	std::vector<float> noiseRegion = cachedNoiseRegion;

	std::vector<std::shared_ptr<Chunk>> newChunks;
	// Pass 1: Construct chunks (doesn't generate 'em)
	for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
		for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
			glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};
			// Make sure to not overrwrite the chunk if it already exists
			// chunks.try_emplace(chunkPos, std::make_shared<Chunk>(chunkPos));
			auto [it, inserted] = chunks.try_emplace(chunkPos, std::make_shared<Chunk>(chunkPos));
			if (inserted) {
				newChunks.push_back(it->second);
			}
		}
	}

	// Pass 2: Generate the block data from the noise for all the chunks
	for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
		for (int dz = -renderDistance; dz <= renderDistance; ++dz) {

			glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};
			glm::vec3  worldPos = Chunk::chunkToWorld(chunkPos);

			int offsetX = worldPos.x - noiseOrigin.x;
			int offsetZ = worldPos.z - noiseOrigin.z;

			Chunk* chunk = getChunk(worldPos);

			if (chunk->state == ChunkState::Empty) {
				Timer terrain_timer("Chunk terrain generation");
				chunk->generate(std::span{noiseRegion}, region.x, offsetX, offsetZ);
				chunk->state = ChunkState::Generated;
			}
		}
	}

	// Pass 3: Link neighbors
	/*
	   for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
	   for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
	   glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};
	   glm::vec3 worldPos = Chunk::chunkToWorld(chunkPos);

	   Chunk* chunk = getChunk(worldPos);
	   if (!chunk) {
	   log::system_error("ChunkManager", "chunk at {} not found for neighbor linking", glm::to_string(chunk->getPos()));
	   continue;
	   }

	   auto getNeighbor = [&](int ndx, int ndz) -> std::shared_ptr<Chunk> {
	   if (ndx < -renderDistance || ndx > renderDistance || ndz < -renderDistance || ndz > renderDistance)
	   return nullptr;

	   glm::ivec3 neighborChunkPos{playerChunkPos.x + ndx, 0, playerChunkPos.z + ndz};
	   auto it = chunks.find(neighborChunkPos);
	   if (it != chunks.end())
	   return it->second;
	   else
	   return nullptr;
	   };


	   Timer timer3("Neighbor chunk linking");
	   chunk->leftChunk  = getNeighbor(dx - 1, dz);
	   chunk->rightChunk = getNeighbor(dx + 1, dz);
	   chunk->backChunk  = getNeighbor(dx, dz - 1);
	   chunk->frontChunk = getNeighbor(dx, dz + 1);
	   }
	   }
	   */

	for (auto& chunk : newChunks) {
		glm::ivec3 pos = chunk->getPos();

		auto linkNeighbor = [&](int dx, int dz,
		                        std::weak_ptr<Chunk>& side,
		                        auto                  setOtherSide) {
			glm::ivec3 neighborPos = pos + glm::ivec3{dx, 0, dz};
			auto       it          = chunks.find(neighborPos);
			if (it != chunks.end()) {
				side = it->second;               // weak_ptr assignment
				setOtherSide(it->second, chunk); // pass shared_ptr
			}
		};

		linkNeighbor(-1, 0, chunk->leftChunk, [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->rightChunk = c; });
		linkNeighbor(1, 0, chunk->rightChunk, [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->leftChunk = c; });
		linkNeighbor(0, -1, chunk->backChunk, [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->frontChunk = c; });
		linkNeighbor(0, 1, chunk->frontChunk, [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->backChunk = c; });
		chunk->state = ChunkState::Linked;
	}

	// Pass 4: Neighbor-dependent generation (trees, water, etc)
	for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
		for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
			glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};
			glm::vec3  worldPos = Chunk::chunkToWorld(chunkPos);

			Chunk* chunk   = getChunk(worldPos);
			int    offsetX = (dx + renderDistance) * CHUNK_SIZE.x;
			int    offsetZ = (dz + renderDistance) * CHUNK_SIZE.z;

			Timer neighbor_timer("Neighboring chunks checks");
			chunk->updateMesh();

			/*
			if (neighborsAreGenerated(chunk) && chunk->state == ChunkState::Linked) {
				chunk->genWaterPlane(std::span{noiseRegion}, region.x, offsetX, offsetZ);
				chunk->updateMesh();

				chunk->updateNeighborMeshes();
			}
			*/
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
	glm::ivec3 localPos = Chunk::worldToChunk(worldPos);
	auto       it       = chunks.find(localPos);
	if (it != chunks.end()) {
		return it->second.get(); // Return raw pointer (Chunk*)
	} else {
#if defined(DEBUG)
		log::system_error("ChunkManager", "Chunk at {} not found!", glm::to_string(localPos));
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
