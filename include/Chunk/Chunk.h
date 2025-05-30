#pragma once
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "../../defines.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <PerlinNoise.hpp>

class Shader;
struct Block {
    enum class blocks { AIR, DIRT, GRASS, STONE, LAVA, WATER, WOOD, MAX_BLOCKS };	// Up to 512 blocks
    blocks type = blocks::AIR;

    // Compile-time function for converting enum to string
    static constexpr const char* toString(blocks type) {
	switch (type) {
	    case blocks::AIR:   return "AIR";
	    case blocks::DIRT:  return "DIRT";
	    case blocks::GRASS: return "GRASS";
	    case blocks::STONE: return "STONE";
	    case blocks::LAVA:  return "LAVA";
	    case blocks::WATER: return "WATER";
	    case blocks::WOOD:  return "WOOD";
	    default:            return "UNKNOWN BLOCK TYPE";
	}
    }
    static constexpr const char* toString(int type) {
	switch (type) {
	    case 0:	return "AIR";
	    case 1:	return "DIRT";
	    case 2:	return "GRASS";
	    case 3:	return "STONE";
	    case 4:	return "LAVA";
	    case 5:	return "WATER";
	    case 6:	return "WOOD";
	    default:	return "UNKNOWN BLOCK TYPE";
	}
    }

    // Instance method that delegates to static constexpr function
    constexpr const char* toString(void) const {
	return toString(type);
    }
    
    // Compile-time function for converting enum to int
    static constexpr int toInt(blocks type) {
	return static_cast<int>(type);
    }

    // Instance method that delegates to static constexpr function
    constexpr int toInt(void) const {
	return toInt(type);
    }
};

struct Face {
    int32_t position;
    int32_t tex_coord;	// Bits 0-9: u, 10-19: v, 20-22: face_id, 23-31: block_type

    // Constructor
    Face(int vx, int vy, int vz, int tex_u, int tex_v, int face, int block_type) {
	position = (vx & 0x3FF) | ((vy & 0x3FF) << 10) | ((vz & 0x3FF) << 20);
	tex_coord = (tex_u & 0x3FF) | ((tex_v & 0x3FF) << 10) | ((face & 0x7) << 20) | ((block_type & 0x1FF) << 23);
    }
};
class Chunk
{
    public:
	glm::ivec3 position;  // Store chunk grid position (chunkX, chunkY, chunkZ)
	glm::ivec3 _size;	// Must be a power of 2
	Chunk(const glm::ivec3& chunkPos, glm::ivec3 size);
	~Chunk(void);
	std::vector<Block>& getChunkData(void) { return chunkData; }
	Block getBlockAt(int x, int y, int z) const
	{
	    return chunkData[getBlockIndex(x, y, z)];
	}
	//std::vector<int> getChunkVertices(void) { return faces; };
	std::vector<int> getChunkVertices(void) {
	    std::vector<int> intVertices;
	    intVertices.reserve(faces.size() * 2);  // Pre-allocate space for the faces

	    // Iterate through each Face and extract its data into the integer vector
	    for (const Face& v : faces) {
		intVertices.emplace_back(v.position);
		intVertices.emplace_back(v.tex_coord);
	    }

	    return intVertices;
	}
	void generate(const std::vector<float>& noiseMap);
	void updateMesh(void);
	//Function to generate vertex data for a single Block
	void generateBlockFace(const Block& block, int x, int y, int z, std::vector<Face>& faces);
	//Function to get the index of a block in the Chunk
	inline int getBlockIndex(int x, int y, int z) const noexcept {
#ifdef DEBUG
	    if (x < 0 || x >= _size.x || y < 0 || y >= _size.y || z < 0 || z >= _size.z) {
		std::cerr << "ACCESSED INDEX OUT OF BOUNDS FOR CURRENT CHUNK!\n";
		return -1;
	    }
#endif
	    // Return the calculated index, using bit-shifting for efficiency
	    return x + (y << logSizeX) + (z << (logSizeX + logSizeY));
	}

	//Function to render a chunk
	void renderChunk(std::unique_ptr<Shader>  &shader);

	bool isFaceVisible(const Block& block, int x, int y, int z) const;
	// Convert world coordinates to chunk coordinates
	static glm::ivec3 worldToChunk(const glm::vec3& worldPos, const glm::vec3& chunkSize) {
	    return glm::ivec3(
		    static_cast<int>(std::floor(worldPos.x / chunkSize.x)),
		    static_cast<int>(std::floor(worldPos.y / chunkSize.y)),
		    static_cast<int>(std::floor(worldPos.z / chunkSize.z))
		    );
	}

	// Get local coordinates within a chunk
	static glm::ivec3 worldToLocal(const glm::vec3& worldPos, const glm::vec3& chunkSize) {
	    glm::ivec3 chunkPos = worldToChunk(worldPos, chunkSize);
	    glm::vec3 chunkOrigin = chunkToWorld(chunkPos, chunkSize);

	    return glm::ivec3(
		    static_cast<int>(std::floor(worldPos.x - chunkOrigin.x)),
		    static_cast<int>(std::floor(worldPos.y - chunkOrigin.y)),
		    static_cast<int>(std::floor(worldPos.z - chunkOrigin.z))
		    );
	}

	// Convert chunk coordinates back to world coordinates (chunk's origin position)
	static glm::vec3 chunkToWorld(const glm::ivec3& chunkPos, const glm::vec3& chunkSize) {
	    return glm::vec3(
		    chunkPos.x * chunkSize.x,
		    chunkPos.y * chunkSize.y,
		    chunkPos.z * chunkSize.z
		    );
	}
	//static glm::ivec3 worldToChunk(const glm::vec3& worldPos, const glm::vec3& chunkSize);
	//static glm::ivec3 worldToLocal(const glm::vec3& worldPos, const glm::vec3& chunkSize);
	//static glm::vec3 chunkToWorld(const glm::ivec3& chunkPos, const glm::vec3& chunkSize);
	bool isAir(int x, int y, int z) const{
	    if (x < 0 || x >= _size.x || y < 0 || y >= _size.y || z < 0 || z >= _size.z) {
		return true; // Out of bounds = Air
	    }
	    return chunkData[getBlockIndex(x, y, z)].type == Block::blocks::AIR;
	}

	glm::mat4 getModelMatrix(void) const {
	    return glm::translate(glm::mat4(1.0f), glm::vec3(position.x * _size.x, position.y * _size.y, position.z * _size.z));
	}

	void cleanup(void) {
	    if(SSBO)	glDeleteBuffers(1, &SSBO);
	}

	// References to neighboring chunks
	std::weak_ptr<Chunk> leftChunk;   // -x direction
	std::weak_ptr<Chunk> rightChunk;  // +x direction
	std::weak_ptr<Chunk> frontChunk;  // +z direction
	std::weak_ptr<Chunk> backChunk;   // -z direction

	GLuint SSBO, EBO;
	int nonAirBlockCount = 0;
	int blockCount = 0;
	void uploadData(void);
	int getStartX(void) const { return position.x * _size.x; }
	int getStartZ(void) const { return position.z * _size.z; }
	int getStartY(void) const { return position.y * _size.y; }
	int logSizeX;
	int logSizeY;
	std::vector<Face> faces;
	std::vector<Block> chunkData;
	std::vector<unsigned int> indices;
	int bufferSize;

};
