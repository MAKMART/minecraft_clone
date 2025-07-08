#include "Chunk.h"

#include <cstdlib>
#include <ctime>
#include <memory>
#include <stdexcept>

#include "../Timer.h"
#include "Shader.h"
#include "defines.h"

Chunk::Chunk(const glm::ivec3 &chunkPos)
    : position(chunkPos), SSBO(0) {
    try {
        chunkData.resize(chunkSize.x * chunkSize.y * chunkSize.z);
    } catch (const std::bad_alloc &e) {
        throw std::runtime_error("Failed to allocate chunkData vector: " + std::string(e.what()));
    }

    // Construct AABB
    glm::vec3 worldOrigin = chunkToWorld(position, chunkSize);
    glm::vec3 worldMax = worldOrigin + glm::vec3(chunkSize);
    aabb = AABB(worldOrigin, worldMax);
    glCreateBuffers(1, &SSBO);
    srand(static_cast<unsigned int>(position.x ^ position.y ^ position.z));
}
Chunk::~Chunk() {
    if (SSBO)
        glDeleteBuffers(1, &SSBO);
}
void Chunk::generateTreeAt(int x, int y, int z) {
    const int trunkHeight = 4 + rand() % 2; // height 4 or 5
    const int leafRadius = 2;

    // Place trunk
    for (int i = 0; i < trunkHeight; ++i) {
        int index = getBlockIndex(x, y + i, z);
        if (index != -1) {
            chunkData[index].type = Block::blocks::WOOD;
        }
    }

    // Place leaves - simple cube or spherical cap
    for (int dy = -leafRadius; dy <= leafRadius; ++dy) {
        for (int dx = -leafRadius; dx <= leafRadius; ++dx) {
            for (int dz = -leafRadius; dz <= leafRadius; ++dz) {
                if (dx * dx + dy * dy + dz * dz <= leafRadius * leafRadius + 1) {
                    int lx = x + dx;
                    int ly = y + trunkHeight + dy;
                    int lz = z + dz;
                    // Only place leaves on Block::blocks::AIR
                    if (getBlockAt(lx, ly, lz).type == Block::blocks::AIR)
                        setBlockAt(lx, ly, lz, Block::blocks::LEAVES);
                }
            }
        }
    }
}

void Chunk::generate(const std::vector<float> &noiseMap) {
    Timer generation_timer("Chunk::generate");

    constexpr int dirtDepth = 3;
    constexpr int beachDepth = 3; // How wide the beach is vertically

    for (int z = 0; z < chunkSize.z; ++z) {
        for (int x = 0; x < chunkSize.x; ++x) {
            int noiseIndex = z * chunkSize.x + x;
#if defined(DEBUG)
            if (noiseIndex < 0 || noiseIndex >= static_cast<int>(noiseMap.size())) {
                log::system_warn("Chunk", "Noise index out of bounds: {}", noiseIndex);
                continue;
            }
#endif
            int height = static_cast<int>(noiseMap[noiseIndex] * chunkSize.y);
            height = std::clamp(height, 0, chunkSize.y - 1);

            for (int y = 0; y < chunkSize.y; ++y) {
                int index = getBlockIndex(x, y, z);
                if (index == -1) continue;

                if (y > height) {
                    chunkData[index].type = Block::blocks::AIR;
                    continue;
                }

                if (y == height) {
                    if (height <= seaLevel + beachDepth) {
                        chunkData[index].type = Block::blocks::SAND; // Beach top
                    } else {
                        chunkData[index].type = Block::blocks::GRASS;
                    }
                } else if (y >= height - dirtDepth) {
                    if (height <= seaLevel + beachDepth) {
                        chunkData[index].type = Block::blocks::SAND; // Beach body
                    } else {
                        chunkData[index].type = Block::blocks::DIRT;
                    }
                } else {
                    chunkData[index].type = Block::blocks::STONE;
                }
            }
        }
    }

    // Water pass
    for (int z = 0; z < chunkSize.z; ++z) {
        for (int x = 0; x < chunkSize.x; ++x) {
            int noiseIndex = z * chunkSize.x + x;
            int height = static_cast<int>(noiseMap[noiseIndex] * chunkSize.y);
            height = std::clamp(height, 0, chunkSize.y - 1);

            // Fill from height up to sea level with water (if below sea level)
            for (int y = height + 1; y <= seaLevel && y < chunkSize.y; ++y) {
                int index = getBlockIndex(x, y, z);
                if (index == -1)
                    continue;
                Block &block = chunkData[index];
                if (block.type == Block::blocks::AIR)
                    setBlockAt(x, y, z, Block::blocks::WATER);
            }
        }
    }

}
void Chunk::genTrees(const std::vector<float> &noiseMap) {
    Timer tree_timer("trees_pass");
    // Tree pass
    for (int z = 0; z < chunkSize.z; ++z) {
        for (int x = 0; x < chunkSize.x; ++x) {
            int noiseIndex = z * chunkSize.x + x;
            int height = static_cast<int>(noiseMap[noiseIndex] * chunkSize.y);
            height = std::clamp(height, 0, chunkSize.y - 1);

            // Rough condition for tree placement: 1 in 20 chance
            int index = getBlockIndex(x, height, z);
            if (index == -1)
                continue;
            if (chunkData[index].type == Block::blocks::GRASS && height > seaLevel && rand() % 80 == 0) {
                generateTreeAt(x, height + 1, z); // +1 so it grows *on top* of the grass
            }
        }
    }
}
void Chunk::uploadData() {
    if (faces.empty() && waterFaces.empty()) {
        log::system_error("Chunk", "Empty combined face buffer for chunk ({}, {}, {})", position.x, position.y, position.z);
        return;
    }

    if (SSBO == 0) {
        glCreateBuffers(1, &SSBO);
    }

    opaqueFaceCount = static_cast<GLuint>(faces.size());
    transparentFaceCount = static_cast<GLuint>(waterFaces.size());

    std::vector<Face> combinedFaces;
    combinedFaces.reserve(faces.size() + waterFaces.size());
    combinedFaces.insert(combinedFaces.end(), faces.begin(), faces.end());
    combinedFaces.insert(combinedFaces.end(), waterFaces.begin(), waterFaces.end());

    glNamedBufferData(SSBO, combinedFaces.size() * sizeof(Face), combinedFaces.data(), GL_DYNAMIC_DRAW);
    //glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);
}

void Chunk::updateMesh() {
    faces.clear();

    // Pass 1: Blocks from seaLevel to chunkSize.y — use generateBlockFace
    for (int z = 0; z < chunkSize.z; ++z) {
        for (int x = 0; x < chunkSize.x; ++x) {
            int maxHeight = 0;
            for (int y = seaLevel; y < chunkSize.y; ++y) {
                int index = getBlockIndex(x, y, z);
                if (index == -1)
                    continue;
                if (chunkData[index].type != Block::blocks::AIR && chunkData[index].type != Block::blocks::WATER) {
                    maxHeight = y;
                }
            }
            for (int y = seaLevel; y <= maxHeight; ++y) {
                int index = getBlockIndex(x, y, z);
                if (index == -1)
                    continue;
                const Block &block = chunkData[index];
                generateBlockFace(block, x, y, z);
            }
        }
    }

    // Pass 2: Blocks from 0 to seaLevel — use generateSeaBlockFace
    for (int z = 0; z < chunkSize.z; ++z) {
        for (int x = 0; x < chunkSize.x; ++x) {
            for (int y = 0; y < seaLevel; ++y) {
                int index = getBlockIndex(x, y, z);
                if (index == -1)
                    continue;
                const Block &block = chunkData[index];
                generateSeaBlockFace(block, x, y, z);
            }
        }
    }

    uploadData();

}
// Helper function to check if a face under the water is visible
bool Chunk::isSeaFaceVisible(const Block &block, int x, int y, int z) {
    // Face is visible ONLY if neighbor block is NOT transparent
    const Block::blocks type = getBlockAt(x, y, z).type;
    return type == Block::blocks::WATER || type == Block::blocks::LEAVES;
}
void Chunk::generateSeaBlockFace(const Block &block, int x, int y, int z) {
    // Lambda to push a face into the vector.
    auto pushFace = [this](int x, int y, int z, int u, int v, int face,
                           int block_type) {
        this->waterFaces.emplace_back(Face(x, y, z, u, v, face, block_type));
    };

    uint8_t visibilityMask =
        (isSeaFaceVisible(block, x, y, z + 1) << 0) | // Front Face
        (isSeaFaceVisible(block, x, y, z - 1) << 1) | // Back Face
        (isSeaFaceVisible(block, x - 1, y, z) << 2) | // Left Face
        (isSeaFaceVisible(block, x + 1, y, z) << 3) | // Right Face
        (isSeaFaceVisible(block, x, y + 1, z) << 4) | // Top Face
        (isSeaFaceVisible(block, x, y - 1, z) << 5);  // Bottom Face

    int topX, topY, botX, botY, X, Y, topandBotX, topandBotY;
    switch (block.type) {
    case Block::blocks::AIR:
        return; // No geometry for AIR
    case Block::blocks::DIRT:
        X = 0, Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::GRASS:
        topX = 1;
        topY = 1;
        botX = 0;
        botY = 0;
        X = 1;
        Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, topX, topY, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, botX, botY, 5, block.toInt());
        break;
    case Block::blocks::STONE:
        X = 0, Y = 1;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::LAVA:
        X = 5, Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::WATER:
        X = 0, Y = 4;
        //if (visibilityMask & (1 << 0))
          //  this->waterFaces.emplace_back(Face(x, y, z, X, Y, 0, block.toInt()));
        //if (visibilityMask & (1 << 1))
          //  this->waterFaces.emplace_back(Face(x, y, z, X, Y, 1, block.toInt()));
        //if (visibilityMask & (1 << 2))
          //  this->waterFaces.emplace_back(Face(x, y, z, X, Y, 2, block.toInt()));
        //if (visibilityMask & (1 << 3))
          //  this->waterFaces.emplace_back(Face(x, y, z, X, Y, 3, block.toInt()));
        if (visibilityMask & (1 << 4))
          //  this->waterFaces.emplace_back(Face(x, y, z, X, Y, 4, block.toInt()));
          pushFace(x, y, z, X, Y, 4, block.toInt());
        //if (visibilityMask & (1 << 5))
          //  this->waterFaces.emplace_back(Face(x, y, z, X, Y, 5, block.toInt()));
        break;
    case Block::blocks::WOOD:
        X = 2;
        Y = 0;
        topandBotX = 2;
        topandBotY = 1;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, topandBotX, topandBotY, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, topandBotX, topandBotY, 5, block.toInt());
        break;
    case Block::blocks::LEAVES:
        X = 0, Y = 2;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::SAND:
        X = 4, Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    default:
        log::system_error("Chunk", "UNHANDLED BLOCK CASE: {} # {}", block.toString(), block.toInt());
        break;
    }
}
// Helper function to check if a face is visible
bool Chunk::isFaceVisible(const Block &block, int x, int y, int z) {
    return Block::isTransparent(getBlockAt(x, y, z).type);
}
void Chunk::generateBlockFace(const Block &block, int x, int y, int z) {
    // Lambda to push a face into the vector.
    auto pushFace = [this](int x, int y, int z, int u, int v, int face,
                           int block_type) {
        this->faces.emplace_back(Face(x, y, z, u, v, face, block_type));
    };

    uint8_t visibilityMask =
        (isFaceVisible(block, x, y, z + 1) << 0) | // Front Face
        (isFaceVisible(block, x, y, z - 1) << 1) | // Back Face
        (isFaceVisible(block, x - 1, y, z) << 2) | // Left Face
        (isFaceVisible(block, x + 1, y, z) << 3) | // Right Face
        (isFaceVisible(block, x, y + 1, z) << 4) | // Top Face
        (isFaceVisible(block, x, y - 1, z) << 5);  // Bottom Face

    int topX, topY, botX, botY, X, Y, topandBotX, topandBotY;
    switch (block.type) {
    case Block::blocks::AIR:
        return; // No geometry for AIR
    case Block::blocks::DIRT:
        X = 0, Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::GRASS:
        topX = 1;
        topY = 1;
        botX = 0;
        botY = 0;
        X = 1;
        Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, topX, topY, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, botX, botY, 5, block.toInt());
        break;
    case Block::blocks::STONE:
        X = 0, Y = 1;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::LAVA:
        X = 5, Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::WATER:
        X = 0, Y = 4;
        if (visibilityMask & (1 << 0))
            this->waterFaces.emplace_back(Face(x, y, z, X, Y, 0, block.toInt()));
        if (visibilityMask & (1 << 1))
            this->waterFaces.emplace_back(Face(x, y, z, X, Y, 1, block.toInt()));
        if (visibilityMask & (1 << 2))
            this->waterFaces.emplace_back(Face(x, y, z, X, Y, 2, block.toInt()));
        if (visibilityMask & (1 << 3))
            this->waterFaces.emplace_back(Face(x, y, z, X, Y, 3, block.toInt()));
        if (visibilityMask & (1 << 4))
            this->waterFaces.emplace_back(Face(x, y, z, X, Y, 4, block.toInt()));
        if (visibilityMask & (1 << 5))
            this->waterFaces.emplace_back(Face(x, y, z, X, Y, 5, block.toInt()));
        break;
    case Block::blocks::WOOD:
        X = 2;
        Y = 0;
        topandBotX = 2;
        topandBotY = 1;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, topandBotX, topandBotY, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, topandBotX, topandBotY, 5, block.toInt());
        break;
    case Block::blocks::LEAVES:
        X = 0, Y = 2;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    case Block::blocks::SAND:
        X = 4, Y = 0;
        if (visibilityMask & (1 << 0))
            pushFace(x, y, z, X, Y, 0, block.toInt());
        if (visibilityMask & (1 << 1))
            pushFace(x, y, z, X, Y, 1, block.toInt());
        if (visibilityMask & (1 << 2))
            pushFace(x, y, z, X, Y, 2, block.toInt());
        if (visibilityMask & (1 << 3))
            pushFace(x, y, z, X, Y, 3, block.toInt());
        if (visibilityMask & (1 << 4))
            pushFace(x, y, z, X, Y, 4, block.toInt());
        if (visibilityMask & (1 << 5))
            pushFace(x, y, z, X, Y, 5, block.toInt());
        break;
    default:
        log::system_error("Chunk", "UNHANDLED BLOCK CASE: {} # {}", block.toString(), block.toInt());
        break;
    }
}
void Chunk::renderOpaqueMesh(std::unique_ptr<Shader> &shader) {
    shader->setMat4("model", getModelMatrix());
    if (!faces.empty()) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);
        DrawArraysWrapper(GL_TRIANGLES, 0, opaqueFaceCount * 6);
    }

}
void Chunk::renderTransparentMesh(std::unique_ptr<Shader> &shader) {
    shader->setMat4("model", getModelMatrix());
    if (!waterFaces.empty()) {
        glDisable(GL_CULL_FACE);
        if (BLENDING) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);
        DrawArraysWrapper(GL_TRIANGLES, opaqueFaceCount * 6, transparentFaceCount * 6);
        // Restore GL state if needed
        if (FACE_CULLING) {
            glEnable(GL_CULL_FACE);
        }
    }
}
