#include "ChunkManager.h"
#include "defines.h"
#include <cstdint>
#include <stdexcept>
#include <glm/glm.hpp>

ChunkManager::ChunkManager(glm::ivec3 size, int renderDistance, std::optional<siv::PerlinNoise::seed_type> seed) : chunkSize(size) { 
    // Initialize Perlin noise
    if (seed.has_value()) {
	perlin = siv::PerlinNoise(seed.value());
	std::cout << "ChunkManager initialized with seed: " << seed.value() << "\n";
    } else {
	std::random_device rd;
	std::mt19937 engine(rd());
	std::uniform_int_distribution<int> distribution(1, 999999);
	siv::PerlinNoise::seed_type random_seed = distribution(engine);
	perlin = siv::PerlinNoise(random_seed);
	std::cout << "ChunkManager initialized with random seed: " << random_seed << "\n";
    }
    if (!chunksTexture.get())
	chunksTexture = std::make_unique<Texture>(BLOCK_ATLAS_TEXTURE_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);
    if(!chunkShader.get())
	chunkShader = std::make_unique<Shader>(CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY);

    if (chunkSize.x <= 0 || chunkSize.y <= 0 || chunkSize.z <= 0) {
	throw std::invalid_argument("chunkSize must be positive");
    }
    if ((chunkSize.x & (chunkSize.x - 1)) != 0 || (chunkSize.y & (chunkSize.y - 1)) != 0 || (chunkSize.z & (chunkSize.z - 1)) != 0) {
	throw std::invalid_argument("chunkSize must be a power of 2");
    }
    uint64_t totalSize = static_cast<uint64_t>(chunkSize.x) * chunkSize.y * chunkSize.z;
    if (totalSize > 1'000'000) { // Arbitrary limit, adjust as needed
	throw std::invalid_argument("chunkSize too large: " + std::to_string(totalSize) + " elements");
    }

    lastChunkX = -1;  // Initialize with a value that is not equal to any valid chunk position.
    lastChunkZ = -1;
    
    glCreateVertexArrays(1, &VAO);

    loadChunksAroundPlayer({0.0f, 0.0f, 0.0f}, renderDistance);
}
ChunkManager::~ChunkManager(void) { 
    clearChunks();
}

// Unload a chunk at the given world position
void ChunkManager::unloadChunk(glm::vec3 worldPos) {
    auto chunkKey = getChunkKey(worldPos);
    auto it = chunks.find(chunkKey);
    if (it == chunks.end()) return;

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

    // Clean up and remove the chunk
    chunk->cleanup();
    chunks.erase(it);
}

void ChunkManager::updateBlock(glm::vec3 worldPos, Block::blocks newType) {
    auto chunk = getChunk(worldPos);
    if (!chunk) return;

    glm::ivec3 localPos = Chunk::worldToLocal(worldPos, chunkSize);
    int index = chunk->getBlockIndex(localPos.x, localPos.y, localPos.z);
    if (index == -1) return;

    chunk->getChunkData()[index].type = newType;
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

void ChunkManager::loadChunksAroundPlayer(glm::vec3 playerPosition, int renderDistance)
{
    if (renderDistance <= 0)
        return;

    int playerChunkX = static_cast<int>(floor(playerPosition.x / chunkSize.x));
    int playerChunkZ = static_cast<int>(floor(playerPosition.z / chunkSize.z));

    for (int dx = -renderDistance; dx <= renderDistance; ++dx)
    {
	for (int dz = -renderDistance; dz <= renderDistance; ++dz)
	{
	    int chunkX = playerChunkX + dx;
	    int chunkZ = playerChunkZ + dz;

	    std::tuple<int, int, int> chunkKey = {chunkX, 0, chunkZ}; // Avoid std::make_tuple()
	    try {
		auto [it, inserted] = chunks.emplace(chunkKey, std::make_shared<Chunk>(glm::ivec3(chunkX, 0, chunkZ), chunkSize));
		if (!inserted) continue; // Chunk already exists
		// Generate noise map
		noiseMap.resize(chunkSize.x * chunkSize.z);

		for (int z = 0; z < chunkSize.z; ++z) {
		    for (int x = 0; x < chunkSize.x; ++x) {
			float worldX = (chunkX * chunkSize.x + x);
			float worldZ = (chunkZ * chunkSize.z + z);
			float frequency = 0.005f;
			float rawNoise = LayeredPerlin(worldX, worldZ, 7, 0.003f, 1.2f); // returns in [-some, +some]
			float redistributed = pow(abs(rawNoise), 1.3f) * (int)glm::sign(rawNoise);
			noiseMap[z * chunkSize.x + x] = redistributed;
		    }
		}
		std::shared_ptr<Chunk> newChunk = chunks[chunkKey];
		// Generate chunk data
		newChunk->generate(noiseMap);

		// Set neighbor references for the new chunk
		newChunk->leftChunk = getChunk({(chunkX - 1) * chunkSize.x, 0, chunkZ * chunkSize.z});
		newChunk->rightChunk = getChunk({(chunkX + 1) * chunkSize.x, 0, chunkZ * chunkSize.z});
		newChunk->frontChunk = getChunk({chunkX * chunkSize.x, 0, (chunkZ + 1) * chunkSize.z});
		newChunk->backChunk = getChunk({chunkX * chunkSize.x, 0, (chunkZ - 1) * chunkSize.z});

		// Update neighbor references for adjacent chunks
		if (auto left = newChunk->leftChunk.lock()) {
		    left->rightChunk = newChunk;
		    left->updateMesh();
		}
		if (auto right = newChunk->rightChunk.lock()) {
		    right->leftChunk = newChunk;
		    right->updateMesh();
		}
		if (auto front = newChunk->frontChunk.lock()) {
		    front->backChunk = newChunk;
		    front->updateMesh();
		}
		if (auto back = newChunk->backChunk.lock()) {
		    back->frontChunk = newChunk;
		    back->updateMesh();
		}


	    }
	    catch (const std::exception& e) {
		std::cerr << "Failed to create chunk at (" << chunkX << ", 0, " << chunkZ << "): " << e.what() << std::endl;
	    }
	}
    }
}
void ChunkManager::unloadDistantChunks(glm::vec3 playerPosition, int unloadDistance)
{
    int playerChunkX = static_cast<int>(floor(playerPosition.x / chunkSize.x));
    int playerChunkY = static_cast<int>(floor(playerPosition.y / chunkSize.y));
    (void)playerChunkY;
    int playerChunkZ = static_cast<int>(floor(playerPosition.z / chunkSize.z));

    int unloadDistSquared = unloadDistance * unloadDistance; // Avoid sqrt()

    std::erase_if(chunks, [&](const auto& pair) {
        auto [chunkX, chunkY, chunkZ] = pair.first;

        int distX = chunkX - playerChunkX;
        int distZ = chunkZ - playerChunkZ;
        int distSquared = distX * distX + distZ * distZ;

        return distSquared > unloadDistSquared; // Remove if too far
    });
}
// Get a chunk by its world position
std::shared_ptr<Chunk> ChunkManager::getChunk(glm::vec3 worldPos) const
{
    std::tuple<int, int, int> chunkPosition = getChunkKey(worldPos);
    if (chunks.find(chunkPosition) != chunks.end()) {
        return chunks.find(chunkPosition)->second;
    } else {
#ifdef DEBUG
        //std::cerr << "Chunk at " << chunkX << ", " << chunkY << ", " << chunkZ << " not found!" << std::endl;
#endif
        return nullptr;
    }
}
// Function returning a raw pointer (Chunk*) with a flag
Chunk* ChunkManager::getChunk(glm::vec3 worldPos, bool returnRawPointer) const
{
    (void)returnRawPointer;
    std::tuple<int, int, int> chunkPosition = getChunkKey(worldPos);
    auto [chunkX, chunkY, chunkZ] = chunkPosition;
    auto it = chunks.find(chunkPosition);
    if (it != chunks.end())
    {
	return it->second.get(); // Return raw pointer (Chunk*)
    }
    else
    {
#ifdef DEBUG
	std::cerr << "Chunk at " << chunkX << ", " << chunkY << ", " << chunkZ << " not found!" << std::endl;
#endif
	return nullptr;
    }
}

void ChunkManager::renderChunks(glm::vec3 player_position, unsigned int render_distance, const Camera& camera)
{

    chunkShader->use();
    chunkShader->setMat4("projection", camera.GetProjectionMatrix());
    chunkShader->setMat4("view", camera.GetViewMatrix());

    // Calculate the player's current chunk position (X, Z)
    int playerChunkX = static_cast<int>(floor(player_position.x / chunkSize.x));
    int playerChunkZ = static_cast<int>(floor(player_position.z / chunkSize.z));

    // If the player has moved to a new chunk, update loaded chunks
    if (playerChunkX != lastChunkX || playerChunkZ != lastChunkZ)
    {
        loadChunksAroundPlayer(player_position, render_distance);
        if (render_distance > 0)
            unloadDistantChunks(player_position, render_distance + 3);

        lastChunkX = playerChunkX;
        lastChunkZ = playerChunkZ;
    }

    // Bind texture once for all chunks (reduces state changes)
    chunksTexture->Bind(0);

    glBindVertexArray(VAO);
    // Optimize chunk rendering loop
    for (const auto& [key, chunk] : chunks) {
        if (!chunk) continue;

        // Extract chunk position by reference (avoids unnecessary copying)
        const int& x = std::get<0>(key);
        const int& y = std::get<1>(key);
        const int& z = std::get<2>(key);

        glm::vec3 worldPos = glm::vec3(x * chunkSize.x, y * chunkSize.y, z * chunkSize.z);

        // Check if chunk is within camera view
        if (!camera.isChunkVisible(chunk->getAABB())) continue;
        // Render the chunk with the current shader
        chunk->renderChunk(chunkShader);
    }
    glBindVertexArray(0);
    chunksTexture->Unbind(0);

}
