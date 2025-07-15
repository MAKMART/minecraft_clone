#include "ChunkManager.h"
#include "defines.h"
#include <cmath>
#include <stdexcept>
#include "Timer.h"
#include "logger.hpp"
#include <unordered_set>

ChunkManager::ChunkManager(int renderDistance, std::optional<siv::PerlinNoise::seed_type> seed) {
    // Initialize Perlin noise
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
    if (!chunksTexture.get())
        chunksTexture = std::make_unique<Texture>(BLOCK_ATLAS_TEXTURE_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);
    if (!chunkShader.get())
        chunkShader = std::make_unique<Shader>("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY);
    if (!waterShader.get())
        waterShader = std::make_unique<Shader>("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY);   

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

    // Initialize with a value that is not equal to any valid chunk position.
    lastChunkX = -99999999;
    lastChunkZ = -99999999;

    glCreateVertexArrays(1, &VAO);

    loadChunksAroundPlayer({0.0f, 0.0f, 0.0f}, renderDistance);
}
ChunkManager::~ChunkManager() {
    clearChunks();
}

// Unload a chunk at the given world position
void ChunkManager::unloadChunk(glm::vec3 worldPos) {
    auto chunkKey = getChunkKey(worldPos);
    auto it = chunks.find(chunkKey);
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

    // Clean up and remove the chunk
    chunks.erase(it);
}

void ChunkManager::updateBlock(glm::vec3 worldPos, Block::blocks newType) {
    auto chunk = getChunk(worldPos);
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
bool ChunkManager::neighborsAreGenerated(const std::shared_ptr<Chunk> &chunk) {
    return isGenerated(chunk->leftChunk.lock()) &&
           isGenerated(chunk->rightChunk.lock()) &&
           isGenerated(chunk->frontChunk.lock()) &&
           isGenerated(chunk->backChunk.lock());
}

bool ChunkManager::isGenerated(const std::shared_ptr<Chunk>& chunk) {
    return chunk && chunk->state >= ChunkState::Generated;
}

void ChunkManager::loadChunksAroundPlayer(glm::vec3 playerPosition, int renderDistance) {
    Timer chunksTimer("chunk_generation");
    if (renderDistance <= 0)
        return;

    const int playerChunkX = static_cast<int>(floor(playerPosition.x / chunkSize.x));
    const int playerChunkZ = static_cast<int>(floor(playerPosition.z / chunkSize.z));

    // Define the total region size based on render distance and chunk size
    const int regionWidth = (2 * renderDistance + 1) * chunkSize.x;
    const int regionHeight = (2 * renderDistance + 1) * chunkSize.z;
    std::vector<float> noiseRegion(regionWidth * regionHeight);
    std::vector<float> chunkNoise(chunkSize.x * chunkSize.z);
    std::unordered_set<std::shared_ptr<Chunk>> updatedChunks; // Track chunks needing mesh updates

    // Precompute noise for the entire region
    {
        Timer noiseTimer("Noise precomputation");
        for (int z = 0; z < regionHeight; ++z) {
            for (int x = 0; x < regionWidth; ++x) {
                const float wx = static_cast<float>((playerChunkX - renderDistance) * chunkSize.x + x);
                const float wz = static_cast<float>((playerChunkZ - renderDistance) * chunkSize.z + z);
                const float rawNoise = LayeredPerlin(wx, wz, 7, 0.003f, 1.2f);
                noiseRegion[z * regionWidth + x] = std::pow(std::abs(rawNoise), 1.3f) * glm::sign(rawNoise);
            }
        }
    }


    for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
        for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
            const int chunkX = playerChunkX + dx;
            const int chunkZ = playerChunkZ + dz;
            const int worldX = chunkX * chunkSize.x;
            const int worldZ = chunkZ * chunkSize.z;
            const int regionX = (dx + renderDistance) * chunkSize.x;
            const int regionZ = (dz + renderDistance) * chunkSize.z;

            auto chunkPtr = getOrCreateChunk({worldX, 0, worldZ});

            // Stage 1: Terrain generation using precomputed noise
            if (chunkPtr->state == ChunkState::Empty) {
                Timer terrainTimer("Chunk terrain generation");
                // Extract the relevant noise subregion for this chunk
                for (int cz = 0; cz < chunkSize.z; ++cz) {
                    for (int cx = 0; cx < chunkSize.x; ++cx) {
                        chunkNoise[cz * chunkSize.x + cx] = noiseRegion[(regionZ + cz) * regionWidth + (regionX + cx)];
                    }
                }
                chunkPtr->generate(chunkNoise);
                chunkPtr->state = ChunkState::Generated;
                updatedChunks.insert(chunkPtr);
            }

            // Stage 2: Neighbor linking with caching
            {
                Timer linkTimer("Neighbor chunk linking");
                auto createNeighbor = [&](int offsetX, int offsetZ) {
                    return getOrCreateChunk({(chunkX + offsetX) * chunkSize.x, 0, (chunkZ + offsetZ) * chunkSize.z});
                };
                chunkPtr->leftChunk = createNeighbor(-1, 0);
                chunkPtr->rightChunk = createNeighbor(1, 0);
                chunkPtr->frontChunk = createNeighbor(0, 1);
                chunkPtr->backChunk = createNeighbor(0, -1);
            }

            // Stage 3: Tree placement and neighbor updates if neighbors are ready
            if (neighborsAreGenerated(chunkPtr) && chunkPtr->state == ChunkState::Generated) {
                Timer neighborTimer("Neighboring chunks checks");
                // Reuse the same chunkNoise subregion for trees
                chunkPtr->genTrees(chunkNoise);
                chunkPtr->state = ChunkState::TreesPlaced;
                updatedChunks.insert(chunkPtr);

                // Batch neighbor updates
                std::array<std::pair<std::shared_ptr<Chunk>, std::shared_ptr<Chunk>>, 4> neighbors = {
                    { {chunkPtr->leftChunk.lock(), chunkPtr}, {chunkPtr->rightChunk.lock(), chunkPtr},
                      {chunkPtr->frontChunk.lock(), chunkPtr}, {chunkPtr->backChunk.lock(), chunkPtr} }
                };
                for (const auto& [neighbor, selfPtr] : neighbors) {
                    if (neighbor) {
                        // Compare raw pointers to check identity
                        if (selfPtr.get() == neighbor->rightChunk.lock().get()) neighbor->leftChunk = selfPtr;
                        else if (selfPtr.get() == neighbor->leftChunk.lock().get()) neighbor->rightChunk = selfPtr;
                        else if (selfPtr.get() == neighbor->frontChunk.lock().get()) neighbor->backChunk = selfPtr;
                        else if (selfPtr.get() == neighbor->backChunk.lock().get()) neighbor->frontChunk = selfPtr;
                        updatedChunks.insert(neighbor);
                    }
                }
            }
        }
    }

    // Batch mesh updates
    {
        Timer meshTimer("Mesh updates");
        for (auto chunk : updatedChunks) {
            chunk->updateMesh();
        }
    }
}




void ChunkManager::unloadDistantChunks(glm::vec3 playerPosition, int unloadDistance) {
    int playerChunkX = static_cast<int>(floor(playerPosition.x / chunkSize.x));
    int playerChunkY = static_cast<int>(floor(playerPosition.y / chunkSize.y));
    (void)playerChunkY;
    int playerChunkZ = static_cast<int>(floor(playerPosition.z / chunkSize.z));

    int unloadDistSquared = unloadDistance * unloadDistance; // Avoid sqrt()

    std::erase_if(chunks, [&](const auto &pair) {
        auto [chunkX, chunkY, chunkZ] = pair.first;

        int distX = chunkX - playerChunkX;
        int distZ = chunkZ - playerChunkZ;
        int distSquared = distX * distX + distZ * distZ;

        return distSquared > unloadDistSquared; // Remove if too far
    });
}
// Get or create a chunk if failed to getChunk
std::shared_ptr<Chunk> ChunkManager::getOrCreateChunk(glm::vec3 worldPos) {
    auto chunkPosition = getChunkKey(worldPos);
    auto [it, inserted] = chunks.try_emplace(chunkPosition, nullptr);

    if (!inserted && it->second) {
        return it->second;
    }

    auto [chunkX, chunkY, chunkZ] = chunkPosition;
    auto& chunkPtr = it->second;
    chunkPtr = std::make_shared<Chunk>(glm::ivec3(chunkX, chunkY, chunkZ));
    return chunkPtr;
}
// Get a chunk by its world position
std::shared_ptr<Chunk> ChunkManager::getChunk(glm::vec3 worldPos) const {
    std::tuple<int, int, int> chunkPosition = getChunkKey(worldPos);
    auto [chunkX, chunkY, chunkZ] = chunkPosition;
    if (chunks.find(chunkPosition) != chunks.end()) {
        return chunks.find(chunkPosition)->second;
    } else {
#if defined(DEBUG)
        log::system_error("ChunkManager", "Chunk at {}, {}, {} not found!", chunkX, chunkY, chunkZ);
#endif
        return nullptr;
    }
}
// Function returning a raw pointer (Chunk*) with a flag
Chunk *ChunkManager::getChunk(glm::vec3 worldPos, bool returnRawPointer) const {
    (void)returnRawPointer;
    std::tuple<int, int, int> chunkPosition = getChunkKey(worldPos);
    auto [chunkX, chunkY, chunkZ] = chunkPosition;
    auto it = chunks.find(chunkPosition);
    if (it != chunks.end()) {
        return it->second.get(); // Return raw pointer (Chunk*)
    } else {
#if defined(DEBUG)
        log::system_error("ChunkManager", "Chunk at {}, {}, {} not found!", chunkX, chunkY, chunkZ);
#endif
        return nullptr;
    }
}

void ChunkManager::renderChunks(const glm::vec3& player_position, unsigned int render_distance, const CameraController &cam_ctrl, float time) {
    chunkShader->use();
    chunkShader->setMat4("projection", cam_ctrl.getProjectionMatrix());
    chunkShader->setMat4("view", cam_ctrl.getViewMatrix());
    chunkShader->setFloat("time", time);

    int playerChunkX = static_cast<int>(floor(player_position.x / chunkSize.x));
    int playerChunkZ = static_cast<int>(floor(player_position.z / chunkSize.z));

    if (playerChunkX != lastChunkX || playerChunkZ != lastChunkZ) {
        if  (render_distance > 0)
            loadChunksAroundPlayer(player_position, render_distance);
        if (render_distance > 0)
            unloadDistantChunks(player_position, render_distance + 3);

        lastChunkX = playerChunkX;
        lastChunkZ = playerChunkZ;
    }

    chunksTexture->Bind(0);
    glBindVertexArray(VAO);

    // Separate opaque and transparent lists
    std::vector<std::shared_ptr<Chunk>> opaqueChunks;
    std::vector<std::shared_ptr<Chunk>> transparentChunks;

    for (const auto& [key, chunk] : chunks) {
        if (!chunk)
            continue;

        if (!cam_ctrl.isAABBVisible(chunk->getAABB()))  // If the current chunk's AABB is outside the camera's frustum (a.k.a can't be seen), continue on with life
            continue;
        if (chunk->hasOpaqueMesh())
            opaqueChunks.emplace_back(chunk);
        if (chunk->hasTransparentMesh())
            transparentChunks.emplace_back(chunk);
    }

    // Render opaque
    for (auto& chunk : opaqueChunks) {
        chunk->renderOpaqueMesh(chunkShader);
    }

    glDisable(GL_CULL_FACE);
    if (BLENDING) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    // Render transparent
    for (auto& chunk : transparentChunks) {
        chunk->renderTransparentMesh(chunkShader);
    }
    // Restore GL state if needed
    if (FACE_CULLING) {
        glEnable(GL_CULL_FACE);
    }


    glBindVertexArray(0);
    chunksTexture->Unbind(0);
}

