#pragma once
#include <cstdint>
#include <filesystem>


// Project Attributes
extern std::string WINDOW_NAME;
extern const std::string VERSION;

#ifndef GL_TYPES_DEFINED
#define GL_TYPES_DEFINED
typedef unsigned int GLenum;
typedef int          GLint;
typedef int          GLsizei;
#endif

#define TEX_SCALE 1023.0f

// ANSI escape codes for color
#define RESET_COLOR "\033[0m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define GREEN "\033[32m"
#define CYAN "\033[36m"


#define DrawArraysWrapper(mode, first, count) { \
    glDrawArrays(mode, first, count); \
    g_drawCallCount++; \
    GLenum error = glGetError(); \
    if (error != GL_NO_ERROR) { \
        std::cerr << "OpenGL Error in DrawArraysWrapper at " << __FILE__ << ":" << __LINE__ \
                  << " Error code: " << std::hex << error << std::dec << std::endl; \
    } \
}

#define DrawElementsWrapper(mode, count, type, indices) { \
    glDrawElements(mode, count, type, indices); \
    g_drawCallCount++; \
    GLenum error = glGetError(); \
    if (error != GL_NO_ERROR) { \
        std::cerr << "OpenGL Error in DrawElementsWrapper at " << __FILE__ << ":" << __LINE__ \
                  << " Error code: " << std::hex << error << std::dec << std::endl; \
    } \
}
#define DrawArraysInstancedWrapper(mode, first, count, instanceCount) { \
    glDrawArraysInstanced(mode, first, count, instanceCount); \
    g_drawCallCount++; \
    GLenum error = glGetError(); \
    if (error != GL_NO_ERROR) { \
        std::cerr << "OpenGL Error in DrawElementsWrapper at " << __FILE__ << ":" << __LINE__ \
                  << " Error code: " << std::hex << error << std::dec << std::endl; \
    } \
}

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
extern std::filesystem::path CHUNK_VERTEX_SHADER_DIRECTORY;
extern std::filesystem::path CHUNK_FRAGMENT_SHADER_DIRECTORY;
extern std::filesystem::path CROSSHAIR_VERTEX_SHADER_DIRECTORY;
extern std::filesystem::path CROSSHAIR_FRAGMENT_SHADER_DIRECTORY;
// 	Assets
extern std::filesystem::path ASSETS_DIRECTORY;
extern std::filesystem::path WINDOW_ICON_DIRECTORY;
extern std::filesystem::path BLOCK_ATLAS_TEXTURE_DIRECTORY;
extern std::filesystem::path DEFAULT_SKIN_DIRECTORY;
extern std::filesystem::path ICONS_DIRECTORY;


// World Attributes
extern float GRAVITY;

// Utils for rendering
extern unsigned int g_drawCallCount;
extern void GLCheckError(const char* file, int line);
extern float LINE_WIDTH;
extern bool WIREFRAME_MODE;
extern bool DEPTH_TEST;
extern uint8_t DEPTH_BITS;
extern uint8_t STENCIL_BITS;
extern bool BLENDING;
extern bool V_SYNC;
extern bool FACE_CULLING;
extern bool MSAA;
extern uint8_t MSAA_STRENGHT;

// Unsigned int types
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// Signed int types
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;


// Floating point types
typedef float f32;
typedef double f64;


// Boolean types
typedef int b32;
typedef bool b8;


// Properly define static assertions
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif


// Ensure all types are of the correct size
STATIC_ASSERT(sizeof(u8)  == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8)  == 1, "Expected i8 to be 1 byte.");
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
