#pragma once
#include "../../defines.h"
#include "AABB.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <PerlinNoise.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include "logger.hpp"

class Shader;

enum class ChunkState {
    Empty,
    Generated,
    TreesPlaced,
    Meshed
};

struct Block {
    enum class blocks : uint8_t {
        AIR     = 0,
        DIRT    = 1,
        GRASS   = 2,
        STONE   = 3,
        LAVA    = 4,
        WATER   = 5,
        WOOD    = 6,
        LEAVES  = 7,
        SAND    = 8,
        MAX_BLOCKS
    }; // Up to 256 blocks
    blocks type = blocks::AIR;

    // Compile-time function for converting enum to string
    static constexpr const char *toString(blocks type) {
        switch (type) {
        case blocks::AIR:
            return "AIR";
        case blocks::DIRT:
            return "DIRT";
        case blocks::GRASS:
            return "GRASS";
        case blocks::STONE:
            return "STONE";
        case blocks::LAVA:
            return "LAVA";
        case blocks::WATER:
            return "WATER";
        case blocks::WOOD:
            return "WOOD";
        case blocks::LEAVES:
            return "LEAVES";
        case blocks::SAND:
            return "SAND";
        default:
            return "UNKNOWN BLOCK TYPE";
        }
    }
    static constexpr const char *toString(int type) {
        switch (type) {
        case 0:
            return "AIR";
        case 1:
            return "DIRT";
        case 2:
            return "GRASS";
        case 3:
            return "STONE";
        case 4:
            return "LAVA";
        case 5:
            return "WATER";
        case 6:
            return "WOOD";
        case 7:
            return "LEAVES";
        case 8:
            return "SAND";
        default:
            return "UNKNOWN BLOCK TYPE";
        }
    }

    static constexpr bool isTransparent(blocks type) {
        return type == Block::blocks::AIR ||
           type == Block::blocks::LEAVES ||
           type == Block::blocks::WATER;
    }

    // Instance method that delegates to static constexpr function
    constexpr const char *toString(void) const { return toString(type); }

    // Compile-time function for converting enum to int
    static constexpr int toInt(blocks type) { return static_cast<int>(type); }

    // Instance method that delegates to static constexpr function
    constexpr int toInt(void) const { return toInt(type); }
};

struct Face {
    uint32_t position;
    uint32_t
        tex_coord; // Bits 0-9: u, 10-19: v, 20-22: face_id, 23-31: block_type

    // Constructor
    Face(int vx, int vy, int vz, int tex_u, int tex_v, int face, int block_type) {
        position = (vx & 0x3FF) | ((vy & 0x3FF) << 10) | ((vz & 0x3FF) << 20);
        tex_coord = (tex_u & 0x3FF) | ((tex_v & 0x3FF) << 10) |
                    ((face & 0x7) << 20) | ((block_type & 0x1FF) << 23);
    }
};

class Chunk {
  public:
    Chunk(const glm::ivec3 &chunkPos);
    ~Chunk();

    std::vector<Block> &getChunkData(void) { return chunkData; }

    bool hasOpaqueMesh() const {
        return !faces.empty();
    }

    bool hasTransparentMesh() const {
        return !waterFaces.empty();
    }

    Block getBlockAt(int x, int y, int z) const {
        // 🌟 Fast path: in-bounds
        if ((uint32_t)x < chunkSize.x &&
                (uint32_t)y < chunkSize.y &&
                (uint32_t)z < chunkSize.z) {
            return chunkData[getBlockIndex(x, y, z)];
        }

        // 🧭 Out-of-bounds — determine neighbor and remap local position
        const Chunk* neighbor = nullptr;
        int nx = x, ny = y, nz = z;

        if (x < 0) {
            neighbor = leftChunk.lock().get();
            nx = chunkSize.x + x;
        } else if (x >= chunkSize.x) {
            neighbor = rightChunk.lock().get();
            nx = x - chunkSize.x;
        } else if (z < 0) {
            neighbor = backChunk.lock().get();
            nz = chunkSize.z + z;
        } else if (z >= chunkSize.z) {
            neighbor = frontChunk.lock().get();
            nz = z - chunkSize.z;
        } else {
            // Y-axis out-of-bounds: no neighbor system for that
            return Block(Block::blocks::AIR);
        }

        // 💨 If neighbor doesn't exist, treat as air
        if (!neighbor) {
            return Block(Block::blocks::AIR);
        }

        // ✅ Recursively query neighbor (no further neighbor chaining)
        return neighbor->getBlockAt(nx, ny, nz);
    }


    bool setBlockAt(int x, int y, int z, Block::blocks type) {
        if (x >= 0 && x < chunkSize.x && y >= 0 && y < chunkSize.y && z >= 0 &&
                z < chunkSize.z) {
            int index = getBlockIndex(x, y, z);
            if (index != -1) {
                if (chunkData[index].type == Block::blocks::AIR &&
                        type != Block::blocks::AIR) {
                }
                chunkData[index].type = type;
                return true;
            }
            return false;
        }

        // OUT OF BOUNDS – resolve neighbor chunk
        std::shared_ptr<Chunk> neighbor;
        glm::ivec3 localPos(x, y, z);

        if (x < 0) {
            neighbor = leftChunk.lock();
            localPos.x = chunkSize.x + x;
        } else if (x >= chunkSize.x) {
            neighbor = rightChunk.lock();
            localPos.x = x - chunkSize.x;
        } else if (z < 0) {
            neighbor = backChunk.lock();
            localPos.z = chunkSize.z + z;
        } else if (z >= chunkSize.z) {
            neighbor = frontChunk.lock();
            localPos.z = z - chunkSize.z;
        } else {
            // y bounds? You can handle this later if you support vertical chunks
            return false;
        }

        if (!neighbor)
            return false;

        return neighbor->setBlockAt(localPos.x, localPos.y, localPos.z, type);
    }

    void generate(const std::vector<float> &noiseMap);

    void updateMesh(void);

    void genTrees(const std::vector<float> &noiseMap);

    void generateSeaBlockFace(const Block &block, int x, int y, int z);

    // Function to generate vertex data for a single Block
    void generateBlockFace(const Block &block, int x, int y, int z);

    // Function to get the index of a block in the Chunk
    inline int getBlockIndex(int x, int y, int z) const noexcept {
#if defined(DEBUG)
        if (x < 0 || x >= chunkSize.x || y < 0 || y >= chunkSize.y || z < 0 ||
            z >= chunkSize.z) {
            log::system_error("Chunk", "ACCESSED INDEX OUT OF BOUNDS FOR CURRENT CHUNK!");
            return -1;
        }
#endif
        // Return the calculated index, using bit-shifting for efficiency
        return x + (y << logSizeX) + (z << (logSizeX + logSizeY));
    }

    glm::ivec3 position; // Store chunk's grid position (chunkX, chunkY, chunkZ)
    AABB aabb;           // Axis Aligned Bounding Box for the chunk
    ChunkState state = ChunkState::Empty;
    AABB getAABB(void) const { return aabb; }

    // Functions to render a chunk
    void renderOpaqueMesh(std::unique_ptr<Shader> &shader);
    void renderTransparentMesh(std::unique_ptr<Shader> &shader);

    bool isSeaFaceVisible(const Block &block, int x, int y, int z);

    bool isFaceVisible(const Block &block, int x, int y, int z);
    // Convert world coordinates to chunk coordinates
    static glm::ivec3 worldToChunk(const glm::vec3 &worldPos,
                                   const glm::vec3 &chunkSize) {
        return glm::ivec3(static_cast<int>(std::floor(worldPos.x / chunkSize.x)),
                          static_cast<int>(std::floor(worldPos.y / chunkSize.y)),
                          static_cast<int>(std::floor(worldPos.z / chunkSize.z)));
    }

    // Get local coordinates within a chunk
    static glm::ivec3 worldToLocal(const glm::vec3 &worldPos,
                                   const glm::vec3 &chunkSize) {
        glm::ivec3 chunkPos = worldToChunk(worldPos, chunkSize);
        glm::vec3 chunkOrigin = chunkToWorld(chunkPos, chunkSize);

        return glm::ivec3(static_cast<int>(std::floor(worldPos.x - chunkOrigin.x)),
                          static_cast<int>(std::floor(worldPos.y - chunkOrigin.y)),
                          static_cast<int>(std::floor(worldPos.z - chunkOrigin.z)));
    }

    // Convert chunk coordinates back to world coordinates (chunk's origin
    // position)
    static glm::vec3 chunkToWorld(const glm::ivec3 &chunkPos,
                                  const glm::vec3 &chunkSize) {
        return glm::vec3(chunkPos.x * chunkSize.x, chunkPos.y * chunkSize.y,
                         chunkPos.z * chunkSize.z);
    }
    bool isAir(int x, int y, int z) const {
        return chunkData[getBlockIndex(x, y, z)].type == Block::blocks::AIR;
    }

    glm::mat4 getModelMatrix(void) const {
        return glm::translate(glm::mat4(1.0f), glm::vec3(position.x * chunkSize.x,
                                                         position.y * chunkSize.y,
                                                         position.z * chunkSize.z));
    }

    void generateTreeAt(int x, int y, int z);

    void cleanup(void) {
        if (SSBO)
            glDeleteBuffers(1, &SSBO);
    }

    // References to neighboring chunks
    std::weak_ptr<Chunk> leftChunk;  // -x direction
    std::weak_ptr<Chunk> rightChunk; // +x direction
    std::weak_ptr<Chunk> frontChunk; // +z direction
    std::weak_ptr<Chunk> backChunk;  // -z direction
    const int seaLevel = 5;
    GLuint SSBO;
    GLuint opaqueFaceCount = 0;
    GLuint transparentFaceCount = 0;
    void uploadData();
    int logSizeX = std::log2(chunkSize.x);
    int logSizeY = std::log2(chunkSize.y);
    std::vector<Face> faces;
    std::vector<Face> waterFaces;
    std::vector<Block> chunkData;
    std::vector<unsigned int> indices;
    glm::mat4 modelMat; // TODO: actually use this model matrix in the class
};
