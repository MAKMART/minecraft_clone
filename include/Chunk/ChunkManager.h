#pragma once
#include <memory>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <tuple>
#include "../Shader.h"
#include "../Texture.h"
#include "Chunk.h"
#include <functional>
#include "../defines.h"
#include <PerlinNoise.hpp>
#include <random>
#include <optional>

class Camera;
namespace std {
template <>
struct hash<std::tuple<int, int, int>> {
    size_t operator()(const std::tuple<int, int, int> &t) const {
        size_t h1 = std::hash<int>{}(std::get<0>(t));
        size_t h2 = std::hash<int>{}(std::get<1>(t));
        size_t h3 = std::hash<int>{}(std::get<2>(t));

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
} // namespace std
struct ivec3Comparator {
    bool operator()(const glm::ivec3 &a, const glm::ivec3 &b) const {
        if (a.x != b.x)
            return a.x < b.x; // Compare x components
        if (a.y != b.y)
            return a.y < b.y; // Compare y components
        return a.z < b.z;     // Compare z components
    }
};

class ChunkManager {
  public:
    // Constructor to initialize the manager
    ChunkManager(int renderDistance, const glm::vec3 &player_pos, std::optional<siv::PerlinNoise::seed_type> seed = std::nullopt);
    // Destructor to clear all the chunks before deleting the ChunkManager
    ~ChunkManager(void);

    std::vector<Block> getChunkData(glm::vec3 worldPos) { return getChunk(worldPos)->getChunkData(); }

    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> getChunks(void) {
        return chunks;
    }

    void updateChunk(glm::vec3 worldPos) {
        Chunk *chunk = getChunk(worldPos, true);
        if (!chunk)
            return;

        chunk->updateMesh(); // Only update the mesh, don't regenerate the terrain
    }

    void loadChunksAroundPlayer(glm::vec3 playerPosition, int renderDistance);

    void unloadDistantChunks(glm::vec3 playerPosition, int unloadDistance);

    void renderChunks(glm::vec3 player_position, unsigned int render_distance, const Camera &camera, float time);

    // Get a chunk by its world position
    std::shared_ptr<Chunk> getChunk(glm::vec3 worldPos) const;

    // Function returning a raw pointer (Chunk*) with a flag
    Chunk *getChunk(glm::vec3 worldPos, bool returnRawPointer) const;

    // Get a chunk and if it fails create it
    std::shared_ptr<Chunk> getOrCreateChunk(glm::vec3 worldPos);

    // Unload a chunk at the given world position
    void unloadChunk(glm::vec3 worldPos);

    // Clear all chunks
    void clearChunks(void) {
        chunks.clear();
    }


    bool neighborsAreGenerated(const std::shared_ptr<Chunk>& chunk);

    bool isGenerated(const std::shared_ptr<Chunk>& chunk);




    void updateBlock(glm::vec3 worldPos, Block::blocks newType);

    bool textureLoaded = false;
    std::unique_ptr<Texture> chunksTexture;
    std::unique_ptr<Shader> chunkShader;
    std::unique_ptr<Shader> waterShader;

  private:
    // Helper to generate a unique key for a chunk position
    std::tuple<int, int, int> getChunkKey(const glm::vec3 &worldPosition) const {
        // Convert world position to chunk indices
        int chunkX = static_cast<int>(std::floor(worldPosition.x / chunkSize.x));
        int chunkY = static_cast<int>(std::floor(worldPosition.y / chunkSize.y));
        int chunkZ = static_cast<int>(std::floor(worldPosition.z / chunkSize.z));
        return std::make_tuple(chunkX, chunkY, chunkZ);
    }
    // Noise map
    std::vector<float> noiseMap;
    // noise
    siv::PerlinNoise perlin;
    int lastChunkX;
    int lastChunkZ;
    // Cached states to minimize redundant updates.
    glm::mat4 lastProjection = glm::mat4(1.0f);
    glm::mat4 lastView = glm::mat4(1.0f);
    // Map to store chunks by their unique keys
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> chunks;

    float LayeredPerlin(float x, float z, int octaves, float baseFreq, float baseAmp, float lacunarity = 2.0f, float persistence = 0.5f);

    // Buffers
    GLuint VAO;
};
