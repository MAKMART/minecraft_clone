#pragma once
#include "graphics/debug_drawer.hpp"
#include "glm/ext/vector_int3.hpp"
#include <cstdint>
#include <filesystem>
#include <string>
#include <cstddef>
#include <cstdio>
#include <string_view>

#ifndef GL_TYPES_DEFINED
#define GL_TYPES_DEFINED
typedef unsigned int GLenum;
typedef int          GLint;
typedef int          GLsizei;
#endif

// ANSI escape codes for color
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define GREEN "\033[32m"
#define CYAN "\033[36m"

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
extern std::filesystem::path WORKING_DIRECTORY;
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
extern float     GRAVITY;
// WARNING: CHUNK_SIZE ≤ 32 because of how the voxel faces are packed in the GPU buffer
inline constexpr static glm::ivec3 CHUNK_SIZE{16};
inline constexpr int MAX_ENTITIES = 10'000;

// Utils for rendering
extern unsigned int g_drawCallCount;
extern void         DrawArraysWrapper(GLenum mode, GLint first, GLsizei count);
extern void         DrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void* indices);
extern void         DrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount);
extern float        LINE_WIDTH;
extern bool         WIREFRAME_MODE;
extern bool         DEPTH_TEST;
extern uint8_t      DEPTH_BITS;
extern GLenum       DEPTH_FUNC;
extern uint8_t      STENCIL_BITS;
extern bool         BLENDING;
extern bool         V_SYNC;
extern bool         FACE_CULLING;
extern bool         MSAA;
extern uint8_t      MSAA_STRENGHT;

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

#define SIZE_OF(x) pretty_size(sizeof(x))

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

// Unsigned int types
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

// Signed int types
typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

// Floating point types
typedef float  f32;
typedef double f64;

// Boolean types
typedef int  b32;
typedef bool b8;

// Properly define static assertions
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Ensure all types are of the correct size
STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

#define TRUE 1
#define FALSE 0

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define KPLATFORM_WINDOWS 1
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
// Linux OS
#define KPLATFORM_LINUX 1
#if defined(__ANDROID__)
#define KPLATFORM_ANDROID 1
#endif
#elif defined(__unix__)
// Catch anything not caught by the above.
#define KPLATFORM_UNIX 1
#elif defined(_POSIX_VERSION)
// Posix
#define KPLATFORM_POSIX 1
#elif __APPLE__
// Apple platforms
#define KPLATFORM_APPLE 1
#include <TargetConditionals.h>
#if TARGET_IPHONE_SIMULATOR
// iOS Simulator
#define KPLATFORM_IOS 1
#define KPLATFORM_IOS_SIMULATOR 1
#elif TARGET_OS_IPHONE
#define KPLATFORM_IOS 1
// iOS device
#elif TARGET_OS_MAC
// Other kinds of Mac OS
#else
#error "Unknown Apple platform"
#endif
#else
#error "Unknown platform!"
#endif
