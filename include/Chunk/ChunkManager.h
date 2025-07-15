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
#include "CameraController.hpp"
#include "Chunk.h"
#include <functional>
#include "../defines.h"
#include <PerlinNoise.hpp>
#include <random>
#include <optional>

class CameraController;
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


class ChunkManager {
  public:
    // Constructor to initialize the manager
    ChunkManager(int renderDistance, std::optional<siv::PerlinNoise::seed_type> seed = std::nullopt);
    // Destructor to clear all the chunks before deleting the ChunkManager
    ~ChunkManager();

    std::vector<Block> getChunkData(glm::vec3 worldPos) { return getChunk(worldPos)->getChunkData(); }

    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> getChunks() {
        return chunks;
    }

    void updateChunk(glm::vec3 worldPos) {
        auto chunk = getChunk(worldPos);
        if (!chunk)
            return;

        chunk->updateMesh(); // Only update the mesh, don't regenerate the terrain
    }

    void loadChunksAroundPlayer(glm::vec3 playerPosition, int renderDistance);

    void unloadDistantChunks(glm::vec3 playerPosition, int unloadDistance);

    void renderChunks(const glm::vec3 &player_position, unsigned int render_distance, const CameraController &cam_ctrl, float time);

    // Get a chunk by its world position
    std::shared_ptr<Chunk> getChunk(glm::vec3 worldPos) const;

    // Function returning a raw pointer (Chunk*) with a flag
    Chunk *getChunk(glm::vec3 worldPos, bool returnRawPointer) const;

    // Get a chunk and if it fails create it
    std::shared_ptr<Chunk> getOrCreateChunk(glm::vec3 worldPos);

    // Unload a chunk at the given world position
    void unloadChunk(glm::vec3 worldPos);

    void clearChunks() {
        chunks.clear();
    }

    bool neighborsAreGenerated(const std::shared_ptr<Chunk> &chunk);

    bool isGenerated(const std::shared_ptr<Chunk>& chunk);

    void updateBlock(glm::vec3 worldPos, Block::blocks newType);

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

    siv::PerlinNoise perlin;

    int lastChunkX;
    int lastChunkZ;
    // Map to store chunks by their unique keys
    std::unordered_map<std::tuple<int, int, int>, std::shared_ptr<Chunk>> chunks;

    float LayeredPerlin(float x, float z, int octaves, float baseFreq, float baseAmp, float lacunarity = 2.0f, float persistence = 0.5f);

    GLuint VAO;
};
