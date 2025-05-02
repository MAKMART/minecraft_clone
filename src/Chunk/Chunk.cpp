#include "Chunk.h"
#include <cmath>
#include <memory>
#include <stdexcept>
#include <winscard.h>
#include "../Timer.h"
#include "defines.h"
#include "Shader.h"

Chunk::Chunk(const glm::ivec3& chunkPos, glm::ivec3 size)
    : position(chunkPos), _size(size), SSBO(0), nonAirBlockCount(0), blockCount(0), bufferSize(0) {
	try {
	    chunkData.resize(_size.x * _size.y * _size.z);
	} catch (const std::bad_alloc& e) {
	    throw std::runtime_error("Failed to allocate chunkData vector: " + std::string(e.what()));
	}

	logSizeX = static_cast<int>(std::log2(size.x));
	logSizeY = static_cast<int>(std::log2(size.y));

	glCreateBuffers(1, &SSBO);
}
Chunk::~Chunk(void)
{
    if(SSBO)	glDeleteBuffers(1, &SSBO);
}
void Chunk::generate(const std::vector<float>& noiseMap) {
    // Reset counters
    nonAirBlockCount = 0;
    blockCount = 0;


    for (int z = 0; z < _size.z; ++z) {
	for (int x = 0; x < _size.x; ++x) {
	    // Calculate height once per (x,z)
	    int noiseIndex = z * _size.x + x;
#ifdef DEBUG
	    if (noiseIndex < 0 || noiseIndex >= static_cast<int>(noiseMap.size())) {
		std::cerr << "Noise index out of bounds: " << noiseIndex << "\n";
		continue;
	    }
#endif
	    int height = static_cast<int>(noiseMap[noiseIndex] * _size.y);
	    height = std::clamp(height, 0, _size.y - 1);
	    for (int y = 0; y < _size.y; ++y) {
		int index = getBlockIndex(x, y, z);
		if(index == -1)	continue;
		if (y <= height) {
		    if (y == height) {
			chunkData[index].type = Block::blocks::GRASS;
			nonAirBlockCount++;
			blockCount++;
                    }
                    else if (y >= height - 3 && height >= 3) {
                        chunkData[index].type = Block::blocks::DIRT;
                        nonAirBlockCount++;
                        blockCount++;
                    }
                    else {
                        chunkData[index].type = Block::blocks::STONE;
                        nonAirBlockCount++;
                        blockCount++;
                    }
                }
                else {
                    chunkData[index].type = Block::blocks::AIR;
                    blockCount++;
                }
            }
        }
    }

    updateMesh();
}

void Chunk::uploadData(void) {
    if (faces.empty()) {
        std::cerr << "Empty faces buffer for chunk (" << position.x << ", " << position.y << ", " << position.z << ")\n";
        return;
    }
    if (SSBO == 0) {
        glCreateBuffers(1, &SSBO);
    }
    // Use glNamedBufferData for a mutable buffer
    glNamedBufferData(SSBO, faces.size() * sizeof(Face), faces.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);
}

void Chunk::updateMesh(void) {
    faces.clear();
    nonAirBlockCount = 0;
    blockCount = 0;

    for (int z = 0; z < _size.z; ++z) {
        for (int x = 0; x < _size.x; ++x) {
            int maxHeight = 0;
            for (int y = 0; y < _size.y; ++y) {
                int index = getBlockIndex(x, y, z);
                if (chunkData[index].type != Block::blocks::AIR) {
                    maxHeight = y;
                }
            }
            for (int y = 0; y <= maxHeight + 1; ++y) {
                int index = getBlockIndex(x, y, z);
                generateBlockFace(chunkData[index], x, y, z, faces);
                if (chunkData[index].type == Block::blocks::AIR) {
                    blockCount++;
                } else {
                    nonAirBlockCount++;
                }
            }
            blockCount += _size.y - (maxHeight + 2);
        }
    }
    uploadData();
}
// Helper function to check if a face is visible
bool Chunk::isFaceVisible(const Block& block, int x, int y, int z) const
{
    (void)block;
    if (x >= 0 && x < _size.x && y >= 0 && y < _size.y && z >= 0 && z < _size.z) {
	return getBlockAt(x, y, z).type == Block::blocks::AIR || getBlockAt(x, y, z).type == Block::blocks::WATER;
    }
    /*
    // Convert local to world coordinates
    glm::vec3 worldPos(position.x + x, position.y + y, position.z + z);
    auto chunk = chunkManager.getChunk(worldPos);
    if (!chunk) return true; // No chunk = air
    glm::ivec3 localPos = Chunk::worldToLocal(worldPos, _size);
    return chunk->getBlockAt(localPos.x, localPos.y, localPos.z).type == Block::blocks::AIR;
    */
    return true;
}
void Chunk::renderChunk(std::unique_ptr<Shader> &shader) {
    if (faces.empty()) 
	return;
    shader->use();
    shader->setMat4("model", getModelMatrix());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO); // Ensure SSBO is bound
    DrawArraysWrapper(GL_TRIANGLES, 0, faces.size() * 6);	// 6 faces per Voxel/Cube
}
void Chunk::generateBlockFace(const Block& block, int x, int y, int z, std::vector<Face>& faces)
{
    // Lambda to push a face into the vector.
    auto pushFace = [&faces](int x, int y, int z, int u, int v, int face, int block_type) {
	faces.emplace_back(Face(x, y, z, u, v, face, block_type));
    };

    uint8_t visibilityMask = 
	(isFaceVisible(block, x, y, z + 1) << 0) |  // Front Face
	(isFaceVisible(block, x, y, z - 1) << 1) |  // Back Face
	(isFaceVisible(block, x - 1, y, z) << 2) |  // Left Face
	(isFaceVisible(block, x + 1, y, z) << 3) |  // Right Face
	(isFaceVisible(block, x, y + 1, z) << 4) |  // Top Face
	(isFaceVisible(block, x, y - 1, z) << 5);   // Bottom Face

    int topX, topY, botX, botY, X, Y, topandBotX, topandBotY;
    switch (block.type)
    {
	case Block::blocks::AIR:
	    return;  // No geometry for AIR
	case Block::blocks::DIRT:
	    X = 0, Y = 0;
	    if(visibilityMask & (1 << 0)) pushFace(x, y, z, X, Y, 0, block.toInt());
	    if(visibilityMask & (1 << 1)) pushFace(x, y, z, X, Y, 1, block.toInt());
	    if(visibilityMask & (1 << 2)) pushFace(x, y, z, X, Y, 2, block.toInt());
	    if(visibilityMask & (1 << 3)) pushFace(x, y, z, X, Y, 3, block.toInt());
	    if(visibilityMask & (1 << 4)) pushFace(x, y, z, X, Y, 4, block.toInt());
	    if(visibilityMask & (1 << 5)) pushFace(x, y, z, X, Y, 5, block.toInt());
	    break;
	case Block::blocks::GRASS:
	    topX = 1;
	    topY = 1;
	    botX = 0;
	    botY = 0;
	    X = 1;
	    Y = 0;
	    if(visibilityMask & (1 << 0)) pushFace(x, y, z, X, Y, 0, block.toInt());
	    if(visibilityMask & (1 << 1)) pushFace(x, y, z, X, Y, 1, block.toInt());
	    if(visibilityMask & (1 << 2)) pushFace(x, y, z, X, Y, 2, block.toInt());
	    if(visibilityMask & (1 << 3)) pushFace(x, y, z, X, Y, 3, block.toInt());
	    if(visibilityMask & (1 << 4)) pushFace(x, y, z, topX, topY, 4, block.toInt());
	    if(visibilityMask & (1 << 5)) pushFace(x, y, z, botX, botY, 5, block.toInt());
	    break;
	case Block::blocks::STONE:
	    X = 0, Y = 1;
	    if(visibilityMask & (1 << 0)) pushFace(x, y, z, X, Y, 0, block.toInt());
	    if(visibilityMask & (1 << 1)) pushFace(x, y, z, X, Y, 1, block.toInt());
	    if(visibilityMask & (1 << 2)) pushFace(x, y, z, X, Y, 2, block.toInt());
	    if(visibilityMask & (1 << 3)) pushFace(x, y, z, X, Y, 3, block.toInt());
	    if(visibilityMask & (1 << 4)) pushFace(x, y, z, X, Y, 4, block.toInt());
	    if(visibilityMask & (1 << 5)) pushFace(x, y, z, X, Y, 5, block.toInt());
	    break;
	case Block::blocks::LAVA:
	    X = 5, Y = 0;
	    if(visibilityMask & (1 << 0)) pushFace(x, y, z, X, Y, 0, block.toInt());
	    if(visibilityMask & (1 << 1)) pushFace(x, y, z, X, Y, 1, block.toInt());
	    if(visibilityMask & (1 << 2)) pushFace(x, y, z, X, Y, 2, block.toInt());
	    if(visibilityMask & (1 << 3)) pushFace(x, y, z, X, Y, 3, block.toInt());
	    if(visibilityMask & (1 << 4)) pushFace(x, y, z, X, Y, 4, block.toInt());
	    if(visibilityMask & (1 << 5)) pushFace(x, y, z, X, Y, 5, block.toInt());
	    break;
	case Block::blocks::WATER:
	    X = 0, Y = 4;
	    if(visibilityMask & (1 << 0)) pushFace(x, y, z, X, Y, 0, block.toInt());
	    if(visibilityMask & (1 << 1)) pushFace(x, y, z, X, Y, 1, block.toInt());
	    if(visibilityMask & (1 << 2)) pushFace(x, y, z, X, Y, 2, block.toInt());
	    if(visibilityMask & (1 << 3)) pushFace(x, y, z, X, Y, 3, block.toInt());
	    if(visibilityMask & (1 << 4)) pushFace(x, y, z, X, Y, 4, block.toInt());
	    if(visibilityMask & (1 << 5)) pushFace(x, y, z, X, Y, 5, block.toInt());
	    break;
	case Block::blocks::WOOD:
	    X = 2;
	    Y = 0;
	    topandBotX = 2;
	    topandBotY = 1;
	    if(visibilityMask & (1 << 0)) pushFace(x, y, z, X, Y, 0, block.toInt());
	    if(visibilityMask & (1 << 1)) pushFace(x, y, z, X, Y, 1, block.toInt());
	    if(visibilityMask & (1 << 2)) pushFace(x, y, z, X, Y, 2, block.toInt());
	    if(visibilityMask & (1 << 3)) pushFace(x, y, z, X, Y, 3, block.toInt());
	    if(visibilityMask & (1 << 4)) pushFace(x, y, z, topandBotX, topandBotY, 4, block.toInt());
	    if(visibilityMask & (1 << 5)) pushFace(x, y, z, topandBotX, topandBotY, 5, block.toInt());
	    break;
	default:
	    std::cerr << "UNHANDLED BLOCK CASE: " << block.toString() << "# " << block.toInt() << std::endl;
	    break;
    }
}
