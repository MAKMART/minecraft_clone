module;
#include <glad/glad.h>
/*
#include <filesystem>
#include <string>
#include <string_view>
*/
export module core;

import std;
import glm;

export {

	// Unsigned int types
	using u8 = unsigned char;
	using u16 = unsigned short;
	using u32 = unsigned int;
	using u64 = unsigned long long;

	// Signed int types
	using i8 = signed char;
	using i16 = signed short;
	using i32 = signed int;
	using i64 = signed long long;

	// Floating point types
	using f32 = float;
	using f64 = double;

	// Boolean types
	using b32 = int;
	using b8 = bool;

	// Controls
	extern uint_fast16_t FORWARD_KEY;
	extern uint_fast16_t BACKWARD_KEY;
	extern uint_fast16_t RIGHT_KEY;
	extern uint_fast16_t LEFT_KEY;
	extern uint_fast16_t JUMP_KEY;
	extern uint_fast16_t UP_KEY;
	extern uint_fast16_t SPRINT_KEY;
	extern uint_fast16_t DOWN_KEY;
	extern uint_fast16_t CROUCH_KEY;
	extern uint_fast16_t MENU_KEY;
	extern uint_fast16_t EXIT_KEY;
	extern uint_fast16_t FULLSCREEN_KEY;
	extern uint_fast16_t WIREFRAME_KEY;
	extern uint_fast16_t SURVIVAL_MODE_KEY;
	extern uint_fast16_t CREATIVE_MODE_KEY;
	extern uint_fast16_t SPECTATOR_MODE_KEY;
	extern uint_fast16_t CAMERA_SWITCH_KEY;
	extern uint_fast8_t  ATTACK_BUTTON;
	extern uint_fast8_t  DEFENSE_BUTTON;

	// Paths
	std::filesystem::path WORKING_DIRECTORY = std::filesystem::current_path();
	// 	Shaders
	extern std::filesystem::path SHADERS_DIRECTORY;

	extern std::filesystem::path PLAYER_VERTEX_SHADER_DIRECTORY;
	extern std::filesystem::path PLAYER_FRAGMENT_SHADER_DIRECTORY;

	extern std::filesystem::path CHUNK_VERTEX_SHADER_DIRECTORY;
	extern std::filesystem::path CHUNK_FRAGMENT_SHADER_DIRECTORY;

	extern std::filesystem::path WATER_VERTEX_SHADER_DIRECTORY;
	extern std::filesystem::path WATER_FRAGMENT_SHADER_DIRECTORY;

	extern std::filesystem::path CROSSHAIR_VERTEX_SHADER_DIRECTORY;
	extern std::filesystem::path CROSSHAIR_FRAGMENT_SHADER_DIRECTORY;

	extern std::filesystem::path UI_VERTEX_SHADER_DIRECTORY;
	extern std::filesystem::path UI_FRAGMENT_SHADER_DIRECTORY;
	// 	Assets
	extern std::filesystem::path ASSETS_DIRECTORY;
	extern std::filesystem::path UI_DIRECTORY;
	extern std::filesystem::path WINDOW_ICON_DIRECTORY;
	extern std::filesystem::path BLOCK_ATLAS_TEXTURE_DIRECTORY;
	extern std::filesystem::path DEFAULT_SKIN_DIRECTORY;
	extern std::filesystem::path ICONS_DIRECTORY;
	extern std::filesystem::path FONTS_DIRECTORY;
	extern std::filesystem::path MAIN_FONT_DIRECTORY;
	extern std::filesystem::path MAIN_DOC_DIRECTORY;

	// World Attributes
	extern float GRAVITY;

	// WARNING: CHUNK_SIZE ≤ 32 because of how the voxel faces are packed in the GPU buffer
	inline constexpr glm::ivec3 CHUNK_SIZE{16};
	static_assert(CHUNK_SIZE.x > 0 && CHUNK_SIZE.y > 0 && CHUNK_SIZE.z > 0, "CHUNK_SIZE must be positive");
	static_assert(
			(CHUNK_SIZE.x & (CHUNK_SIZE.x - 1)) == 0 &&
			(CHUNK_SIZE.y & (CHUNK_SIZE.y - 1)) == 0 &&
			(CHUNK_SIZE.z & (CHUNK_SIZE.z - 1)) == 0,
			"CHUNK_SIZE must be a power of two"
			);
	inline constexpr int SIZE = CHUNK_SIZE.x * CHUNK_SIZE.y * CHUNK_SIZE.z;
	inline constexpr int TOTAL_FACES = SIZE * 6;
	inline constexpr glm::ivec3 LOG_SIZE{
		std::countr_zero(static_cast<unsigned>(CHUNK_SIZE.x)),
		std::countr_zero(static_cast<unsigned>(CHUNK_SIZE.y)),
		std::countr_zero(static_cast<unsigned>(CHUNK_SIZE.z))
	};

	inline glm::ivec3 world_to_chunk(const glm::vec3 &world_pos) noexcept {
		glm::ivec3 block = glm::floor(world_pos); // convert to integer block coordinates
		return {
			block.x >> LOG_SIZE.x,
				block.y >> LOG_SIZE.y,
				block.z >> LOG_SIZE.z
		};
	}

	inline glm::ivec3 world_to_local(const glm::vec3 &world_pos) noexcept {
		glm::ivec3 block = glm::floor(world_pos);
		return {
			block.x & (CHUNK_SIZE.x - 1),
				block.y & (CHUNK_SIZE.y - 1),
				block.z & (CHUNK_SIZE.z - 1)
		};
	}

	inline glm::vec3 chunk_to_world(const glm::ivec3 &chunk_pos) noexcept {
		return glm::vec3{
			chunk_pos.x << LOG_SIZE.x,
				chunk_pos.y << LOG_SIZE.y,
				chunk_pos.z << LOG_SIZE.z
		};
	}


	inline constexpr int MAX_ENTITIES = 10'000;

	// Utils for rendering
	unsigned int g_drawCallCount = 0;
	extern void DrawArraysWrapper(GLenum mode, GLint first, GLsizei count);
	extern void DrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices);
	extern void DrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
	extern void MultiDrawArraysIndirectWrapper(GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);


	inline std::string pretty_size(std::size_t bytes)
	{
		constexpr const char* IEC_UNITS[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};

		double      value      = static_cast<double>(bytes);
		std::size_t unit_index = 0;

		while (value >= 1024.0 && unit_index < (sizeof(IEC_UNITS) / sizeof(*IEC_UNITS)) - 1) {
			value /= 1024.0;
			++unit_index;
		}

		char buf[64];
		if (value - static_cast<long long>(value) < 0.005)
			std::snprintf(buf, sizeof(buf), "%lld %s", static_cast<long long>(value), IEC_UNITS[unit_index]);
		else
			std::snprintf(buf, sizeof(buf), "%.2f %s", value, IEC_UNITS[unit_index]);

		return std::string(buf);
	}

	template <typename T>
		constexpr std::string_view type_name()
		{
#if defined(__clang__) || defined(__GNUC__)
			std::string_view p     = __PRETTY_FUNCTION__;
			auto             start = p.find("T = ") + 4;
			auto             end   = p.find(';', start);
			return p.substr(start, end - start);
#elif defined(_MSC_VER)
			std::string_view p     = __FUNCSIG__;
			auto             start = p.find("type_name<") + 10;
			auto             end   = p.find(">(void)", start);
			return p.substr(start, end - start);
#endif
		}
}
