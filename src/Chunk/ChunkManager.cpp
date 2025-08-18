#include "chunk/ChunkManager.h"
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include "chunk/Chunk.h"
#include "core/Timer.h"
#include <tracy/Tracy.hpp>

ChunkManager::ChunkManager(std::optional<siv::PerlinNoise::seed_type> seed)
	: Atlas(BLOCK_ATLAS_TEXTURE_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST),
	shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
	waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY) {

		log::info("Initializing Chunk Manager...");

		if (chunkSize.x <= 0 || chunkSize.y <= 0 || chunkSize.z <= 0) {
			log::system_error("ChunkManager", "chunkSize < 0");
			std::exit(1);
		}
		if ((chunkSize.x & (chunkSize.x - 1)) != 0 || (chunkSize.y & (chunkSize.y - 1)) != 0 || (chunkSize.z & (chunkSize.z - 1)) != 0) {
			log::system_error("ChunkManager", "chunkSize must be a power of 2");
			std::exit(1);
		}

		if (seed.has_value()) {
			perlin = siv::PerlinNoise(seed.value());
			log::system_info("ChunkManager", "initialized with seed: {}", seed.value());
		} else {
			std::mt19937 engine((std::random_device())());
			std::uniform_int_distribution<int> distribution(1, 999999);
			siv::PerlinNoise::seed_type random_seed = distribution(engine);
			perlin = siv::PerlinNoise(random_seed);
			log::system_info("ChunkManager", "initialized with random seed: {}", random_seed);
		}


		glCreateVertexArrays(1, &VAO);

	}
ChunkManager::~ChunkManager() {
	clearChunks();
}

void ChunkManager::unloadChunk(glm::vec3 worldPos) {
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

void ChunkManager::updateBlock(glm::vec3 worldPos, Block::blocks newType) {
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
	if (localPos.x == chunkSize.x - 1) {
		if (auto neighbor = chunk->rightChunk.lock()) {
			neighbor->updateMesh();
		}
	}
	if (localPos.z == 0) {
		if (auto neighbor = chunk->backChunk.lock()) {
			neighbor->updateMesh();
		}
	}
	if (localPos.z == chunkSize.z - 1) {
		if (auto neighbor = chunk->frontChunk.lock()) {
			neighbor->updateMesh();
		}
	}
	// Add y-boundary checks if needed
}

float ChunkManager::LayeredPerlin(float x, float z, int octaves, float baseFreq, float baseAmp, float lacunarity, float persistence) {
	float total = 0.0f;
	float frequency = baseFreq;
	float amplitude = baseAmp;

	for (int i = 0; i < octaves; ++i) {
		total += (perlin.noise2D(x * frequency, z * frequency)) * amplitude; // Not the _01 variant, so it's in [-1, 1]
		frequency *= lacunarity;
		amplitude *= persistence;
	}

	return total; // raw value, unnormalized
}
bool ChunkManager::neighborsAreGenerated(Chunk* chunk) {
	return chunk &&
		chunk->leftChunk.lock() && chunk->leftChunk.lock()->state >= ChunkState::Generated &&
		chunk->rightChunk.lock() && chunk->rightChunk.lock()->state >= ChunkState::Generated &&
		chunk->frontChunk.lock() && chunk->frontChunk.lock()->state >= ChunkState::Generated &&
		chunk->backChunk.lock() && chunk->backChunk.lock()->state >= ChunkState::Generated;
}
void ChunkManager::loadChunksAroundPos(const glm::ivec3& playerChunkPos, int renderDistance) {

	ZoneScoped;

	Timer chunksTimer("chunk_generation");
	if (renderDistance <= 0) return;

	const int side = 2 * renderDistance + 1;

	glm::ivec2 region = {side * chunkSize.x, side * chunkSize.z};

	// Precomputed noise for the full region
	static std::vector<float> cachedNoiseRegion;
	static glm::ivec2 lastRegionSize = {-1, -1};
	static glm::ivec3 lastNoiseOrigin = {-999999, 0, -999999};

	//static glm::ivec2 lastChunkOrigin = {-99999, -99999};

	const glm::ivec3 noiseOrigin = {
		(playerChunkPos.x - renderDistance) * chunkSize.x,
		0,
		(playerChunkPos.z - renderDistance) * chunkSize.z
	};


	// Pass 1: generate the noise map for all the chunks in the current region that we want to render
	{
		Timer noiseTimer("Noise precomputation");
		if (lastRegionSize != region || lastNoiseOrigin != noiseOrigin) {
			cachedNoiseRegion.resize(region.x * region.y);
			for (int y = 0; y < region.y; ++y) {
				for (int x = 0; x < region.x; ++x) {
					float wx = float((playerChunkPos.x - renderDistance) * chunkSize.x + x);
					float wz = float((playerChunkPos.z - renderDistance) * chunkSize.z + y);
					float rawNoise = LayeredPerlin(wx, wz, 7, 0.003f, 1.2f);
					cachedNoiseRegion[y * region.x + x] = std::pow(std::abs(rawNoise), 1.3f) * glm::sign(rawNoise);
				}
			}
			lastRegionSize = region;
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
			//chunks.try_emplace(chunkPos, std::make_shared<Chunk>(chunkPos));
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
			glm::vec3 worldPos = Chunk::chunkToWorld(chunkPos);


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
				auto setOtherSide) {

			glm::ivec3 neighborPos = pos + glm::ivec3{dx, 0, dz};
			auto it = chunks.find(neighborPos);
			if (it != chunks.end()) {
				side = it->second; // weak_ptr assignment
				setOtherSide(it->second, chunk); // pass shared_ptr
			}
		};

		linkNeighbor(-1,  0, chunk->leftChunk,  [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->rightChunk = c; });
		linkNeighbor( 1,  0, chunk->rightChunk, [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->leftChunk  = c; });
		linkNeighbor( 0, -1, chunk->backChunk,  [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->frontChunk = c; });
		linkNeighbor( 0,  1, chunk->frontChunk, [](std::shared_ptr<Chunk> n, std::shared_ptr<Chunk> c) { n->backChunk  = c; });
		chunk->state = ChunkState::Linked;
	}


	// Pass 4: Neighbor-dependent generation (trees, water, etc)
	for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
		for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
			glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};
			glm::vec3 worldPos = Chunk::chunkToWorld(chunkPos);


			Chunk* chunk = getChunk(worldPos);
			int offsetX = (dx + renderDistance) * chunkSize.x;
			int offsetZ = (dz + renderDistance) * chunkSize.z;

			Timer neighbor_timer("Neighboring chunks checks");
			chunk->updateMesh();

			if (neighborsAreGenerated(chunk) && chunk->state == ChunkState::Linked) {
				chunk->genWaterPlane(std::span{noiseRegion}, region.x, offsetX, offsetZ);
				chunk->genTrees(std::span{noiseRegion}, region.x, offsetX, offsetZ);
				chunk->state = ChunkState::TreesPlaced;
				chunk->updateMesh();

				chunk->updateNeighborMeshes();
			}

		}
	}
}

void ChunkManager::unloadDistantChunks(const glm::ivec3 &playerChunkPos, int unloadDistance) {

	const int unloadDistSquared = unloadDistance * unloadDistance; // Avoid sqrt()

	std::erase_if(chunks, [&](const auto &pair) {
			glm::ivec3 chunkPos = pair.first;

			int distX = chunkPos.x - playerChunkPos.x;
			int distZ = chunkPos.z - playerChunkPos.z;
			int distSquared = distX * distX + distZ * distZ;

			return distSquared > unloadDistSquared; // Remove if too far
			});
}
std::shared_ptr<Chunk> ChunkManager::getOrCreateChunk(glm::vec3 worldPos) {
	glm::ivec3 chunkPos = Chunk::worldToChunk(worldPos);
	auto it = chunks.find(chunkPos);
	if (it != chunks.end() && it->second)
		return it->second;

	auto newChunk = std::make_shared<Chunk>(chunkPos);
	chunks[chunkPos] = newChunk;
	return newChunk;

}

Chunk* ChunkManager::getChunk(glm::vec3 worldPos) const {
	ZoneScoped;
	glm::ivec3 localPos = Chunk::worldToChunk(worldPos);
	auto it = chunks.find(localPos);
	if (it != chunks.end()) {
		return it->second.get(); // Return raw pointer (Chunk*)
	} else {
#if defined(DEBUG)
		log::system_error("ChunkManager", "Chunk at {} not found!", glm::to_string(localPos));
#endif
		return nullptr;
	}
}
void ChunkManager::generateChunks(glm::vec3 playerPos, unsigned int renderDistance) {
	ZoneScoped;
	glm::ivec3 playerChunk{Chunk::worldToChunk(playerPos)};

	if (playerChunk.x != last_player_chunk_pos.x || playerChunk.z != last_player_chunk_pos.z) {
		if  (renderDistance > 0)
			loadChunksAroundPos(playerChunk, renderDistance);
		if (renderDistance > 0)
			unloadDistantChunks(playerChunk, renderDistance + 3);

		last_player_chunk_pos.x = playerChunk.x;
		last_player_chunk_pos.z = playerChunk.z;
	}

}
void ChunkManager::renderChunks(const CameraController &cam_ctrl, float time) {
	ZoneScoped;
	shader.use();
	shader.setMat4("projection", cam_ctrl.getProjectionMatrix());
	shader.setMat4("view", cam_ctrl.getViewMatrix());
	shader.setFloat("time", time);


	Atlas.Bind(0);
	glBindVertexArray(VAO);

	std::vector<std::shared_ptr<Chunk>> opaqueChunks;
	std::vector<std::shared_ptr<Chunk>> transparentChunks;

	for (const auto& [key, chunk] : chunks) {
		if (!chunk)
			continue;

		if (!cam_ctrl.isAABBVisible(chunk->getAABB())) 
			continue;
		if (chunk->hasOpaqueMesh())
			opaqueChunks.emplace_back(chunk);
		if (chunk->hasTransparentMesh())
			transparentChunks.emplace_back(chunk);
	}

	for (auto& chunk : opaqueChunks) {
		chunk->renderOpaqueMesh(shader);
	}

	glDisable(GL_CULL_FACE);
	if (BLENDING) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	std::sort(transparentChunks.begin(), transparentChunks.end(),
			[&](const auto& a, const auto& b) {
			float distA = glm::distance(cam_ctrl.getCurrentPosition(), a->getCenter());
			float distB = glm::distance(cam_ctrl.getCurrentPosition(), b->getCenter());
			return distA > distB; // farthest first
			});


	for (auto& chunk : transparentChunks) {
		chunk->renderTransparentMesh(shader);
	}

	if (FACE_CULLING) {
		glEnable(GL_CULL_FACE);
	}


	glBindVertexArray(0);
	Atlas.Unbind(0);
}
