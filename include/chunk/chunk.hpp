#pragma once
#include "core/aabb.hpp"
#include "core/defines.hpp"
#include "graphics/renderer/shader_storage_buffer.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "core/logger.hpp"
#include "graphics/shader.hpp"
#include <glm/gtx/string_cast.hpp>
#include <FastNoise/FastNoise.h>
#include <bit>

enum class ChunkState { Empty, Generated, Linked, TreesPlaced, Meshed };

struct DrawArraysIndirectCommand {
  GLuint count;         // vertices to draw
  GLuint instanceCount; // usually 1
  GLuint first;         // start index
  GLuint baseInstance;  // usually 0
};

struct Block {
  enum class blocks : std::uint8_t {
    AIR = 0,
    DIRT = 1,
    GRASS = 2,
    STONE = 3,
    LAVA = 4,
    WATER = 5,
    WOOD = 6,
    LEAVES = 7,
    SAND = 8,
    PLANKS = 9,
    MAX_BLOCKS
  }; // Up to 256 blocks
  blocks type = blocks::AIR;

  // Compile-time function for converting enum to string
  static constexpr const char *toString(blocks type) noexcept {
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
  static constexpr const char *toString(int type) noexcept {
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

  static inline constexpr bool isTransparent(blocks type) noexcept {
    return type == Block::blocks::AIR || type == Block::blocks::LEAVES || type == Block::blocks::WATER;
  }

  static inline constexpr bool isLiquid(blocks type) noexcept {
    return type == Block::blocks::WATER || type == Block::blocks::LAVA;
  }

  constexpr const char *toString() const { return toString(type); }

  static constexpr int toInt(blocks type) { return static_cast<int>(type); }

  constexpr int toInt() const { return toInt(type); }
};
static_assert(sizeof(Block) == sizeof(std::uint8_t));

struct face_gpu {
  glm::uvec3 position; // 12 bytes
  uint32_t face_id;    // 4
  uint32_t block_type; // 4
  uint32_t _pad;       // 4 (std430 alignment safety)
  uint32_t _pad2;      // 4 <-- Add these
  uint32_t _pad3;      // 4 <-- to reach 32 bytes
};
static_assert(sizeof(face_gpu) == 32);

class Chunk {
public:
  explicit Chunk(const glm::ivec3 &chunkPos);
  ~Chunk() = default;

  inline const Block& get_block(const glm::ivec3& pos) const noexcept { return blocks[getBlockIndex(pos.x, pos.y, pos.z)]; }

  bool setBlockAt(int x, int y, int z, Block::blocks type) noexcept {
    int index = getBlockIndex(x, y, z);
    if (index == -1) {
      log::system_error("Chunk", "tried to set block out-of-bounds for chunk at {}", glm::to_string(position));
      return false;
    }
	if (blocks[index].type != type) {
		blocks[index].type = type;
		dirty = true;
		return true;
	}
	return false;
  }

  const inline Block *getChunkData() const noexcept { return blocks; }
  const inline glm::ivec3 get_pos() const noexcept{ return position; }
  const inline AABB& getAABB() const noexcept { return aabb; }
  void generate(const FastNoise::SmartNode<FastNoise::FractalFBm>& noise_node, const int SEED) noexcept;
  void render_opaque_mesh(const Shader &shader, GLuint vao) const noexcept;
  void render_transparent_mesh(const Shader &shader) const noexcept;

  inline int getBlockIndex(int x, int y, int z) const noexcept {
#if defined(DEBUG)
    if (x < 0 || x >= CHUNK_SIZE.x || y < 0 || y >= CHUNK_SIZE.y || z < 0 || z >= CHUNK_SIZE.z) {
      log::system_error("Chunk", "Invalid index for block at {}!", glm::to_string(glm::ivec3(x, y, z)));
      return -1;
    }
#endif
    return x + (y << logSizeX) + (z << (logSizeX + logSizeY));
  }

  ChunkState state = ChunkState::Empty;

  constexpr static inline int SIZE = CHUNK_SIZE.x * CHUNK_SIZE.y * CHUNK_SIZE.z;
  constexpr static inline int TOTAL_FACES = SIZE * 6;

  static inline glm::ivec3 world_to_chunk(const glm::vec3 &world_pos) {
	  return glm::ivec3(glm::floor(world_pos / glm::vec3(CHUNK_SIZE)));
  }

  static inline glm::ivec3 world_to_local(const glm::vec3 &world_pos) {
	glm::vec3 chunk_origin = chunk_to_world(world_to_chunk(world_pos));
	return glm::ivec3(glm::floor(world_pos - chunk_origin));
  }

  static inline glm::vec3 chunk_to_world(const glm::ivec3 &chunk_pos) {
    return glm::vec3(chunk_pos * CHUNK_SIZE);
  }

  inline bool isAir(int x, int y, int z) const noexcept {
    return blocks[getBlockIndex(x, y, z)].type == Block::blocks::AIR;
  }

  glm::mat4 getModelMatrix() const {
    return glm::translate(glm::mat4(1.0f), chunk_to_world(position));
  }

  bool dirty = true;
  SSBO block_ssbo;
  SSBO face_flags;
  SSBO faces;
  SSBO prefix;
  SSBO group_totals;
  // unused SSBO normals;
  SSBO indirect_ssbo;
private:
  // Chunk-space position
  glm::ivec3 position;
  AABB aabb;
  const int seaLevel = 5;


  constexpr static int logSizeX = std::countr_zero(static_cast<unsigned>(CHUNK_SIZE.x));
  constexpr static int logSizeY = std::countr_zero(static_cast<unsigned>(CHUNK_SIZE.y));
  float chunk_noise[SIZE];
  Block blocks[SIZE];
};
