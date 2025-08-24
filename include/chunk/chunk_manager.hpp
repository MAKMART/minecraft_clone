#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <tuple>
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "core/camera_controller.hpp"
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include <PerlinNoise.hpp>
#include <optional>
#include "glm/ext/vector_int3.hpp"
#include "core/logger.hpp"
#include <functional>


struct ivec3_hash {
    std::size_t operator()(const glm::ivec3& v) const noexcept {
        std::size_t x = static_cast<std::size_t>(v.x);
        std::size_t y = static_cast<std::size_t>(v.y);
        std::size_t z = static_cast<std::size_t>(v.z);

        const std::size_t prime1 = 73856093;
        const std::size_t prime2 = 19349663;
        const std::size_t prime3 = 83492791;

        return (x * prime1) ^ (y * prime2) ^ (z * prime3);
    }
};

class ChunkManager {
  public:
    ChunkManager(std::optional<siv::PerlinNoise::seed_type> seed = std::nullopt);
    ~ChunkManager();

    Shader& getShader() {
        return shader;
    }

    Shader& getWaterShader() {
        return waterShader;
    }

    const Shader& getShader() const {
        return shader;
    }

    const Shader& getWaterShader() const {
        return waterShader;
    }


    const std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, ivec3_hash>& getChunks() const {
        return chunks;
    }

    void updateBlock(glm::vec3 worldPos, Block::blocks newType);

    void updateChunk(glm::vec3 worldPos) {
        if (auto chunk = getChunk(worldPos))
            chunk->updateMesh();
        else
            return;
    }

    void generateChunks(glm::vec3 playerPos, unsigned int renderDistance);

    void renderChunks(const CameraController &cam_ctrl, float time);

    Chunk* getChunk(glm::vec3 worldPos) const;

    bool getorCreateChunk(glm::vec3 worldPos);

    void unloadChunk(glm::vec3 worldPos);

    void clearChunks() {
        for (auto& [_, chunk] : chunks)
            chunk->breakNeighborLinks();
        chunks.clear();
    }




  private:

    siv::PerlinNoise perlin;
    GLuint VAO;
    Texture Atlas;
    Shader shader;
    Shader waterShader;

    std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, ivec3_hash> chunks;

    bool neighborsAreGenerated(Chunk* chunk);

    // initialize with a value that's != to any reasonable spawn chunk position
    glm::ivec3 last_player_chunk_pos{INT_MIN};        

    std::shared_ptr<Chunk> getOrCreateChunk(glm::vec3 worldPos);

    void loadChunksAroundPos(const glm::ivec3 &playerChunkPos, int renderDistance);

    void unloadDistantChunks(const glm::ivec3 &playerChunkPos, int unloadDistance);

    float LayeredPerlin(float x, float z, int octaves, float baseFreq, float baseAmp, float lacunarity = 2.0f, float persistence = 0.5f);
};
