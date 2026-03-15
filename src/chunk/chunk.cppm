module;
#include <glad/glad.h>
export module chunk;


import core;
import std;
import glm;
import aabb;
import logger;
import ssbo;
import shader;

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

export struct face_gpu {
	std::uint8_t packed;
};
static_assert(sizeof(face_gpu) == sizeof(std::uint8_t));

export class Chunk {
public:
  explicit Chunk(const glm::ivec3& pos)
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
	}
  ~Chunk() = default;

  Block::blocks get_block_type(const int& x, const int& y, const int& z) const noexcept {
	  return static_cast<Block::blocks>(block_types[x + (y << LOG_SIZE.x) + (z << (LOG_SIZE.x + LOG_SIZE.y))]);
  }
  void set_block_type(int x, int y, int z, Block::blocks type) noexcept {
	  int index = get_index(x, y, z);
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

  inline int get_index(const int& x, const int& y, const int& z) const noexcept {
    return x + (y << LOG_SIZE.x) + (z << (LOG_SIZE.x + LOG_SIZE.y));
  }

  bool has_any_blocks() const noexcept { return non_air_count > 0; }

  inline bool isAir(int x, int y, int z) const noexcept {
    return get_block_type(x, y, z) == Block::blocks::AIR;
  }

  // The byte offset within the global_faces buffer where this chunk starts
  GLintptr faces_offset = 0; 

  // The current size (in bytes) of the mesh stored in the global buffer
  // Required so the Free List knows how much to deallocate when remeshing
  GLsizeiptr current_mesh_bytes = 0;

  // Number of faces currently generated for this chunk
  std::uint32_t visible_face_count = 0;

  // Flags for the ChunkManager's update loop
  bool changed = true;       // Set to true initially to force first GPU upload
  bool in_dirty_list = false; // Prevents adding the same chunk to the dirty list twi
  SSBO block_ssbo;
  int non_air_count = 0;
  alignas(16) std::uint8_t block_types[SIZE]{};

  // Chunk-space position
  glm::ivec3 position;
  AABB aabb;
};
