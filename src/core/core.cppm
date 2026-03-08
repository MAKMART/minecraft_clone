module;
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdint>
#include <filesystem>
#include <string>
#include <cstddef>
#include <cstdio>
#include <string_view>
#if defined(DEBUG)
#include "graphics/debug_drawer.hpp"
#endif
#include "core/platform.hpp"
#include <iostream>
export module core;


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
	uint_fast16_t FORWARD_KEY        = GLFW_KEY_W;
	uint_fast16_t BACKWARD_KEY       = GLFW_KEY_S;
	uint_fast16_t RIGHT_KEY          = GLFW_KEY_Q;
	uint_fast16_t LEFT_KEY           = GLFW_KEY_A;
	uint_fast16_t JUMP_KEY           = GLFW_KEY_SPACE;
	uint_fast16_t UP_KEY             = GLFW_KEY_LEFT_SHIFT;
	uint_fast16_t SPRINT_KEY         = GLFW_KEY_LEFT_CONTROL;
	uint_fast16_t DOWN_KEY           = GLFW_KEY_LEFT_CONTROL;
	uint_fast16_t CROUCH_KEY         = GLFW_KEY_LEFT_SHIFT;
	uint_fast16_t MENU_KEY           = GLFW_KEY_F10;
	uint_fast16_t EXIT_KEY           = GLFW_KEY_ESCAPE;
	uint_fast16_t FULLSCREEN_KEY     = GLFW_KEY_F11;
	uint_fast16_t WIREFRAME_KEY      = GLFW_KEY_0;
	uint_fast16_t SURVIVAL_MODE_KEY  = GLFW_KEY_1;
	uint_fast16_t CREATIVE_MODE_KEY  = GLFW_KEY_2;
	uint_fast16_t SPECTATOR_MODE_KEY = GLFW_KEY_3;
	uint_fast16_t CAMERA_SWITCH_KEY  = GLFW_KEY_R;
	uint_fast8_t  ATTACK_BUTTON      = GLFW_MOUSE_BUTTON_LEFT;
	uint_fast8_t  DEFENSE_BUTTON     = GLFW_MOUSE_BUTTON_RIGHT;

	// Paths
	std::filesystem::path WORKING_DIRECTORY = std::filesystem::current_path();
	// 	Shaders
	std::filesystem::path SHADERS_DIRECTORY = WORKING_DIRECTORY / "shaders";

	std::filesystem::path PLAYER_VERTEX_SHADER_DIRECTORY   = SHADERS_DIRECTORY / "player_vert.glsl";
	std::filesystem::path PLAYER_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "player_frag.glsl";

	std::filesystem::path CHUNK_VERTEX_SHADER_DIRECTORY   = SHADERS_DIRECTORY / "chunk_vert.glsl";
	std::filesystem::path CHUNK_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "chunk_frag.glsl";

	std::filesystem::path WATER_VERTEX_SHADER_DIRECTORY   = SHADERS_DIRECTORY / "water_vert.glsl";
	std::filesystem::path WATER_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "water_frag.glsl";

	std::filesystem::path CROSSHAIR_VERTEX_SHADER_DIRECTORY   = SHADERS_DIRECTORY / "crosshair_vert.glsl";
	std::filesystem::path CROSSHAIR_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "crosshair_frag.glsl";

	std::filesystem::path UI_VERTEX_SHADER_DIRECTORY   = SHADERS_DIRECTORY / "ui_vert.glsl";
	std::filesystem::path UI_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "ui_frag.glsl";
	// 	Assets
	std::filesystem::path ASSETS_DIRECTORY              = WORKING_DIRECTORY / "assets";
	std::filesystem::path UI_DIRECTORY                  = ASSETS_DIRECTORY / "UI";
	std::filesystem::path WINDOW_ICON_DIRECTORY         = ASSETS_DIRECTORY / "alpha.png";
	std::filesystem::path BLOCK_ATLAS_TEXTURE_DIRECTORY = ASSETS_DIRECTORY / "block_map.png";
	std::filesystem::path DEFAULT_SKIN_DIRECTORY        = ASSETS_DIRECTORY / "Player/default_skin.png";
	std::filesystem::path ICONS_DIRECTORY               = UI_DIRECTORY / "icons.png";
	std::filesystem::path FONTS_DIRECTORY               = UI_DIRECTORY / "fonts";
	std::filesystem::path MAIN_FONT_DIRECTORY           = FONTS_DIRECTORY / "Hack-Regular.ttf";
	std::filesystem::path MAIN_DOC_DIRECTORY            = UI_DIRECTORY / "main.html";

	// World Attributes
	float GRAVITY = 9.80665f;
	// WARNING: CHUNK_SIZE ≤ 32 because of how the voxel faces are packed in the GPU buffer
	inline constexpr glm::ivec3 CHUNK_SIZE{16};
	STATIC_ASSERT(
			CHUNK_SIZE.x > 0 && CHUNK_SIZE.y > 0 && CHUNK_SIZE.z > 0,
			"CHUNK_SIZE must be positive"
			);

	STATIC_ASSERT(
			(CHUNK_SIZE.x & (CHUNK_SIZE.x - 1)) == 0 &&
			(CHUNK_SIZE.y & (CHUNK_SIZE.y - 1)) == 0 &&
			(CHUNK_SIZE.z & (CHUNK_SIZE.z - 1)) == 0,
			"CHUNK_SIZE must be a power of two"
			);
	inline constexpr int MAX_ENTITIES = 10'000;

	// Utils for rendering
	unsigned int g_drawCallCount = 0;
	void DrawArraysWrapper(GLenum mode, GLint first, GLsizei count)
	{
		glDrawArrays(mode, first, count);
		g_drawCallCount++;
#if defined(DEBUG)
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL Error in DrawArraysWrapper: Error code: "
				<< std::hex << error << std::dec << std::endl;
		}
#endif
	}

	void DrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices)
	{
		glDrawElements(mode, count, type, indices);
		g_drawCallCount++;
#if defined(DEBUG)
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL Error in DrawElementsWrapper: Error code: "
				<< std::hex << error << std::dec << std::endl;
		}
#endif
	}

	void DrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount)
	{
		glDrawArraysInstanced(mode, first, count, instanceCount);
		g_drawCallCount++;
#if defined(DEBUG)
		GLenum error = glGetError();
		if (error != GL_NO_ERROR) {
			std::cerr << "OpenGL Error in DrawArraysInstancedWrapper: Error code: "
				<< std::hex << error << std::dec << std::endl;
		}
#endif
	}
	float   LINE_WIDTH     = 1.5f;
	bool    WIREFRAME_MODE = false;
	bool    DEPTH_TEST     = true;
	uint8_t DEPTH_BITS     = 32;
	GLenum  DEPTH_FUNC     = GL_GREATER;
	uint8_t STENCIL_BITS   = 8;
	bool    BLENDING       = true;
	bool    V_SYNC         = false;
	bool    FACE_CULLING   = true;
	bool    MSAA           = false;
	uint8_t MSAA_STRENGHT  = 4; // 4x MSAA Means 4 samples per pixel. Common options are 2, 4, 8, or 16—higher numbers improve quality but increase GPU memory and computation cost.

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

#if defined(DEBUG)
	inline DebugDrawer& getDebugDrawer()
	{
		static DebugDrawer instance;
		return instance;
	}
#endif
}
