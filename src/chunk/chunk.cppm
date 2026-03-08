module;
#include "core/aabb.hpp"
#include "core/defines.hpp"
//#include <FastNoise/FastNoise.h>
#include "graphics/renderer/shader_storage_buffer.hpp"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include "core/logger.hpp"
#include "graphics/shader.hpp"
#include <bit>
export module chunk;

import glm;

export struct DrawArraysIndirectCommand {
  GLuint count;         // vertices to draw
  GLuint instanceCount; // usually 1
  GLuint first;         // start index
  GLuint baseInstance;  // usually 0
};

export struct Block {
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
	uint packed;
};
static_assert(sizeof(face_gpu) == sizeof(uint));

export class Chunk {
public:
  Chunk(const glm::ivec3& pos)
	  : position(pos), 
	  changed(true),           // Force initial upload
	  in_dirty_list(false), 
	  faces_offset(0), 
	  current_mesh_bytes(0),
	  visible_face_count(0)
	{
		glm::vec3 world = chunk_to_world(position);
		aabb = AABB(world, world + glm::vec3(CHUNK_SIZE));

		block_ssbo = SSBO::Dynamic(nullptr, sizeof(block_types));
		model = glm::translate(glm::mat4(1.0f), chunk_to_world(position));
	}
  ~Chunk() = default;

  Block::blocks get_block_type(int x, int y, int z) const noexcept {
	  return static_cast<Block::blocks>(block_types[x + (y << logSizeX) + (z << (logSizeX + logSizeY))]);
  }
  void set_block_type(int x, int y, int z, Block::blocks type) noexcept {
	  int index = x + (y << logSizeX) + (z << (logSizeX + logSizeY));
	  if (block_types[index] != static_cast<std::uint8_t>(type)) {
		  if (block_types[index] == static_cast<std::uint8_t>(Block::blocks::AIR) &&
				  type != Block::blocks::AIR)
			  ++non_air_count;
		  else if (block_types[index] != static_cast<std::uint8_t>(Block::blocks::AIR) &&
				  type == Block::blocks::AIR)
			  --non_air_count;

		  block_types[index] = static_cast<std::uint8_t>(type);
		  changed = true;
	  }
  }

  inline int get_index(int x, int y, int z) const noexcept {
    return x + (y << logSizeX) + (z << (logSizeX + logSizeY));
  }

  bool has_any_blocks() const noexcept { return non_air_count > 0; }

  constexpr static int SIZE = CHUNK_SIZE.x * CHUNK_SIZE.y * CHUNK_SIZE.z;
  constexpr static int TOTAL_FACES = SIZE * 6;
  constexpr static int logSizeX = std::countr_zero(static_cast<unsigned>(CHUNK_SIZE.x));
  constexpr static int logSizeY = std::countr_zero(static_cast<unsigned>(CHUNK_SIZE.y));

  static inline glm::ivec3 world_to_chunk(const glm::vec3 &world_pos) noexcept {
	  return glm::ivec3(glm::floor(world_pos / glm::vec3(CHUNK_SIZE)));
  }

  static inline glm::ivec3 world_to_local(const glm::vec3 &world_pos) noexcept {
	glm::vec3 chunk_origin = chunk_to_world(world_to_chunk(world_pos));
	return glm::ivec3(glm::floor(world_pos - chunk_origin));
  }

  static inline glm::vec3 chunk_to_world(const glm::ivec3 &chunk_pos) noexcept {
    return glm::vec3(chunk_pos * CHUNK_SIZE);
  }

  inline bool isAir(int x, int y, int z) const noexcept {
    return get_block_type(x, y, z) == Block::blocks::AIR;
  }


  // The byte offset within the global_faces buffer where this chunk starts
  GLintptr faces_offset = 0; 

  // The current size (in bytes) of the mesh stored in the global buffer
  // Required so the Free List knows how much to deallocate when remeshing
  GLsizeiptr current_mesh_bytes = 0;

  // Number of faces currently generated for this chunk
  uint32_t visible_face_count = 0;

  // Flags for the ChunkManager's update loop
  bool changed = true;       // Set to true initially to force first GPU upload
  bool in_dirty_list = false; // Prevents adding the same chunk to the dirty list twi
  SSBO block_ssbo;
  // unused SSBO normals;
  int non_air_count = 0;
  alignas(16) std::uint8_t block_types[SIZE]{};

  // Chunk-space position
  glm::ivec3 position;
  AABB aabb;
  glm::mat4 model;
};
