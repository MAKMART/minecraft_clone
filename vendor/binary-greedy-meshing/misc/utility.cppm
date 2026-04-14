export module utility;

import std;
import mesher;

export {
  struct ivec3 {
    int x;
    int y;
    int z;
  };
	inline int get_zxy_index(int x, int y, int z) {
		return z + (x * CS_P) + (y * CS_P2);
	}

	std::uint32_t get_xyz_key(std::int8_t x, std::int8_t y, std::int8_t z) {
		return (((std::uint32_t)z << 16) | ((std::uint32_t)y << 8) | (std::uint32_t)x);
	}

	ivec3 parse_xyz_key(std::uint32_t key) {
    // Consider static_cast<std::int8_t>(key & 255)
		return ivec3(
				(key & 255),
				(key >> 8 & 255),
				(key >> 16 & 255)
				);
	}
}
