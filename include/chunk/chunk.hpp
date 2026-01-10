#pragma once
#include "core/defines.hpp"
#include "core/aabb.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "graphics/renderer/shader_storage_buffer.hpp"
#include <PerlinNoise.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <system_error>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <memory>
#include <vector>
#include <span>
#include "core/logger.hpp"

class Shader;

enum class ChunkState {
	Empty,
	Generated,
	Linked,
	TreesPlaced,
	Meshed
};

struct Block {
	enum class blocks : std::uint8_t {
		AIR    = 0,
		DIRT   = 1,
		GRASS  = 2,
		STONE  = 3,
		LAVA   = 4,
		WATER  = 5,
		WOOD   = 6,
		LEAVES = 7,
		SAND   = 8,
		PLANKS = 9,
		MAX_BLOCKS
	}; // Up to 256 blocks
	blocks type = blocks::AIR;

	// Compile-time function for converting enum to string
	static constexpr const char* toString(blocks type)
	{
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
		case blocks::PLANKS:
			return "PLANKS";
		default:
			return "UNKNOWN BLOCK TYPE";
		}
	}
	static constexpr const char* toString(int type)
	{
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
		case 9:
			return "PLANKS";
		default:
			return "UNKNOWN BLOCK TYPE";
		}
	}

	static constexpr bool isTransparent(blocks type)
	{
		return type == Block::blocks::AIR ||
		       type == Block::blocks::LEAVES ||
		       type == Block::blocks::WATER;
	}

	constexpr const char* toString() const
	{
		return toString(type);
	}

	static constexpr int toInt(blocks type)
	{
		return static_cast<int>(type);
	}

	constexpr int toInt() const
	{
		return toInt(type);
	}
};

struct Face {
	std::uint32_t position;
	std::uint32_t tex_coord; // Bits:
	                         // 0-9: u,
	                         // 10-19: v,
	                         // 20-22: face_id,
	                         // 23-31: block_type

	Face(int vx, int vy, int vz, int tex_u, int tex_v, int face, int block_type)
	{
		position  = (vx & 0x3FF) | ((vy & 0x3FF) << 10) | ((vz & 0x3FF) << 20);
		tex_coord = (tex_u & 0x3FF) | ((tex_v & 0x3FF) << 10) |
		            ((face & 0x7) << 20) | ((block_type & 0x1FF) << 23);
	}
};

class Chunk
{
	public:
	explicit Chunk(const glm::ivec3& chunkPos);
	~Chunk();

	const Block* getChunkData()
	{
		return blocks;
	}

	const glm::ivec3 getPos() const
	{
		return position;
	}

	bool hasOpaqueMesh() const
	{
		return !faces.empty();
	}

	bool hasTransparentMesh() const
	{
		return !waterFaces.empty();
	}

	glm::vec3 getCenter() const
	{
		return glm::vec3(position) * glm::vec3(CHUNK_SIZE) + glm::vec3(CHUNK_SIZE) * 0.5f;
	}

	Block getBlockAt(int x, int y, int z) const
	{
		int index = getBlockIndex(x, y, z);
		return blocks[index];
	}

	bool setBlockAt(int x, int y, int z, Block::blocks type)
	{
		int index = getBlockIndex(x, y, z);
		if (index == -1) {
			log::system_error("Chunk", "tried to set block out-of-bounds for chunk at {}", glm::to_string(position));
			return false;
		}
		blocks[index].type = type;
		return true;
	}

	void generate(std::span<const float> fullNoise, int regionWidth, int noiseOffsetX, int noiseOffsetZ);

	void updateMesh();

	inline int getBlockIndex(int x, int y, int z) const noexcept
	{
#if defined(DEBUG)
		if (x < 0 || x >= CHUNK_SIZE.x || y < 0 || y >= CHUNK_SIZE.y || z < 0 || z >= CHUNK_SIZE.z) {
			log::system_error("Chunk", "Invalid index for block at {}!", glm::to_string(glm::ivec3(x, y, z)));
			return -1;
		}
#endif
		return x + (y << logSizeX) + (z << (logSizeX + logSizeY));
	}

	ChunkState state = ChunkState::Empty;
	constexpr static int SIZE = CHUNK_SIZE.x * CHUNK_SIZE.y * CHUNK_SIZE.z;
	AABB       getAABB() const
	{
		return aabb;
	}

	void renderOpaqueMesh(const Shader& shader);
	void renderTransparentMesh(const Shader& shader);

	static glm::ivec3 worldToChunk(const glm::vec3& worldPos)
	{
		return glm::ivec3(static_cast<int>(std::floor(worldPos.x / CHUNK_SIZE.x)),
		                  0, // no vertical chunking
		                  static_cast<int>(std::floor(worldPos.z / CHUNK_SIZE.z)));
	}

	static glm::ivec3 worldToLocal(const glm::vec3& worldPos)
	{
		glm::ivec3 chunkPos    = worldToChunk(worldPos);
		glm::vec3  chunkOrigin = chunkToWorld(chunkPos);

		return glm::ivec3(static_cast<int>(std::floor(worldPos.x - chunkOrigin.x)),
		                  static_cast<int>(std::floor(worldPos.y - chunkOrigin.y)),
		                  static_cast<int>(std::floor(worldPos.z - chunkOrigin.z)));
	}

	static glm::vec3 chunkToWorld(const glm::ivec3& chunkPos)
	{
		return glm::vec3(chunkPos * CHUNK_SIZE);
	}

	bool isAir(int x, int y, int z) const
	{
		return blocks[getBlockIndex(x, y, z)].type == Block::blocks::AIR;
	}

	glm::mat4 getModelMatrix() const
	{
		return glm::translate(glm::mat4(1.0f), chunkToWorld(position));
	}

	private:
	void generateBlockFace(const Block& block, int x, int y, int z);

	bool isFaceVisible(int x, int y, int z);

	// Chunk-space position
	glm::ivec3                position;
	AABB                      aabb;
	const int                 seaLevel = 5;
	SSBO			  ssbo;
	GLuint                    opaqueFaceCount      = 0;
	GLuint                    transparentFaceCount = 0;
	void                      uploadData();
	int                       logSizeX;
	int                       logSizeY;
	std::vector<Face>         faces;
	std::vector<Face>         waterFaces;
	alignas(64) Block blocks[SIZE];
	std::vector<unsigned int> indices;
	glm::mat4                 modelMat; // TODO: actually use this model matrix in the class
};
