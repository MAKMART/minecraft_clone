#[[

WARNING: the project build will not succeed unless you modify ${glm_SOURCE_DIR}/glm/glm.cppm

Replace:

// #define GLM_GTC_INLINE_NAMESPACE to inline glm::gtc into glm
// #define GLM_EXT_INLINE_NAMESPACE to inline glm::ext into glm
// #define GLM_GTX_INLINE_NAMESPACE to inline glm::gtx into glm

With:

#define GLM_GTC_INLINE_NAMESPACE to inline glm::gtc into glm
#define GLM_EXT_INLINE_NAMESPACE to inline glm::ext into glm
#define GLM_GTX_INLINE_NAMESPACE to inline glm::gtx into glm

]]

cmake_minimum_required(VERSION 4.2.3)
#set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# ==== C++ Standard ====
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
# This enables 'import std;' globally for all targets using C++23
#set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444")
#set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD ON CACHE BOOL "")
set(CMAKE_CXX_MODULE_STD ON)



set(CMAKE_CXX_COMPILER_LAUNCHER sccache)
project("game" VERSION 1.0.1 LANGUAGES C CXX)

# ==== Build Options ====
option(ENABLE_TRACY "Enable Tracy profiler (Debug only)" OFF)
option(ENABLE_UNITY_BUILD "Enable unity build" OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build everything statically" FORCE)

# ==== Tracy warnings ====
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ENABLE_TRACY)
    message(WARNING "⚠️ Tracy is enabled in Release build — this may reduce performance.")
endif()

# ==== Versioning ====
set(PROJECT_VERSION_SUFFIX "-alpha")
set(PROJECT_FULL_VERSION "${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX}")
set(EXECUTABLE_OUTPUT_NAME "${PROJECT_NAME}-v${PROJECT_FULL_VERSION}")

include(FetchContent)
function(fetch_library name url tag)
	FetchContent_Declare(${name} GIT_REPOSITORY ${url} GIT_TAG ${tag} GIT_SHALLOW TRUE #[[GIT_PROGRESS TRUE]])
	FetchContent_MakeAvailable(${name})
endfunction()

find_package(OpenGL REQUIRED)

if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
endif()

message(STATUS "Build Type       : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard     : ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler ID      : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Version : ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Unity Build      : ${ENABLE_UNITY_BUILD}")
message(STATUS "Tracy Profiler   : ${ENABLE_TRACY}")
message(STATUS "Documentation    : ${ENABLE_DOCS}")
message(STATUS "Executable Name  : ${EXECUTABLE_OUTPUT_NAME}")


# ==== GLFW ====
# Don't know about this
#set(GLFW_USE_WAYLAND TRUE)
# WARNING THIS IS FOR DEBUG WITH RENDERDOC SWITCH TO WAYLAND
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# ==== FreeType ====
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

#string(REPLACE "-fdeps-format=p1689r5" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# ==== RmlUi ====
set(RMLUI_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RMLUI_BUILD_LIBRMLDEBUGGER ON CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "" FORCE)
set(RMLUI_SHELL OFF CACHE BOOL "" FORCE)

# ==== FastNoise2 ====
set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "" FORCE)
set(FASTNOISE2_TESTS OFF CACHE BOOL "" FORCE)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(FASTNOISE2_SIMD_FEATURES "AVX2" CACHE STRING "SIMD level for FastNoise2 in Debug")
else()
    # Full SIMD dispatch in Release builds
    set(FASTNOISE2_SIMD_FEATURES "SSE2;SSE41;AVX2;AVX512" CACHE STRING "SIMD levels for FastNoise2 in Release")
endif()
set(FASTSIMD_FEATURE_SETS "${FASTNOISE2_SIMD_FEATURES}" CACHE STRING "")

# ==== Tracy (optional) ====
if(ENABLE_TRACY)
    set(TRACY_SKIP_GIT_UPDATE ON CACHE BOOL "" FORCE)
    set(TRACY_ENABLE_VERBOSE OFF)
    fetch_library(tracy https://github.com/wolfpld/tracy.git master)
endif()

fetch_library(glm https://github.com/g-truc/glm.git master)
fetch_library(glfw https://github.com/glfw/glfw.git master)
fetch_library(freetype https://github.com/freetype/freetype.git master)
add_library(Freetype::Freetype ALIAS freetype)
fetch_library(RmlUi https://github.com/mikke89/RmlUi.git master)
fetch_library(FastNoise2 https://github.com/Auburn/FastNoise2.git v1.1.1)


# GLAD cmake library
add_library(game_c OBJECT)
target_sources(game_c
	PRIVATE
	$<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/src/external/glad_debug.c>
	$<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/src/external/glad_release.c>
	PUBLIC
		FILE_SET glad_headers
		TYPE HEADERS
		BASE_DIRS ${PROJECT_SOURCE_DIR}/include/external
		FILES
			$<$<CONFIG:Debug>:include/external/glad-debug/glad/glad.h>
			$<$<CONFIG:Release>:include/external/glad-release/glad/glad.h>
)

target_include_directories(game_c PUBLIC
    $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/include/external/glad-debug>
    $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/include/external/glad-release>
)

# ==== IMGUI ====
add_library(imgui STATIC 
    include/external/imgui/imgui.cpp
    include/external/imgui/imgui_draw.cpp
    include/external/imgui/imgui_tables.cpp
    include/external/imgui/imgui_widgets.cpp
    include/external/imgui/backends/imgui_impl_glfw.cpp
    include/external/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${PROJECT_SOURCE_DIR}/include/external/imgui
    ${PROJECT_SOURCE_DIR}/include/external/imgui/backends
    ${glfw_SOURCE_DIR}/include
)
target_link_libraries(imgui PUBLIC glfw)

# ==== Disable debug symbols from target ===
function(disable_debug_symbols target)
    if(NOT TARGET ${target})
        message(STATUS "disable_debug_symbols: target '${target}' does not exist, skipping")
        return()
    endif()

    get_target_property(_aliased ${target} ALIASED_TARGET)
    if(_aliased)
        message(STATUS "disable_debug_symbols: target '${target}' is an ALIAS of '${_aliased}', skipping")
        return()
    endif()

    if(MSVC)
        target_compile_options(${target} PRIVATE /Z7-)
    else()
        target_compile_options(${target} PRIVATE -g0)
    endif()
endfunction()
disable_debug_symbols(imgui)
disable_debug_symbols(glfw)
disable_debug_symbols(freetype)
#disable_debug_symbols(rmlui_core)
#disable_debug_symbols(rmlui_debugger)


# Main executable

add_executable(${PROJECT_NAME})
#target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
set(OUTPUT_BASE_DIR "${CMAKE_BINARY_DIR}/bin")
set_target_properties(${PROJECT_NAME}
	PROPERTIES
	OUTPUT_NAME "${EXECUTABLE_OUTPUT_NAME}"
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}/$<CONFIG>"
)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/graphics/shaders" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

file(GLOB_RECURSE GAME_CORE_CPP_SOURCES
	src/core/*.cpp
	src/graphics/*.cpp
	src/chunk/*.cpp
	src/ui/*.cpp
	src/game/*.cpp
	src/game/ecs/systems/*.cpp
)

target_sources(${PROJECT_NAME}
	PRIVATE
		src/main.cpp
		${GAME_CORE_CPP_SOURCES}
		$<$<BOOL:${ENABLE_TRACY}>:src/tracy_overrides.cpp>
		PUBLIC
		FILE_SET CXX_MODULES 
		FILES
		# for "import glm" support include the glm module file
		${glm_SOURCE_DIR}/glm/glm.cppm
		src/game/ecs/entity.cppm
		src/game/ecs/components.cppm
		src/game/ecs/ecs.cppm
		src/game/ecs/systems/camera_controller_system.cppm
		src/game/ecs/systems/temporal_camera_system.cppm
		src/game/ecs/systems/debug_camera_system.cppm
		src/game/ecs/systems/camera_system.cppm
		src/game/ecs/systems/chunk_renderer_system.cppm
		src/game/ecs/systems/frustum_volume_system.cppm
		src/game/ecs/systems/input_system.cppm
		src/game/ecs/systems/movement_intent_system.cppm
		src/game/ecs/systems/movement_physics_system.cppm
		src/graphics/renderer/framebuffer_manager.cppm
		src/chunk/noise.cppm
		src/chunk/chunk_manager.cppm
		src/chunk/chunk.cppm
		src/core/aabb.cppm
		src/core/logger.cppm
		src/core/raycast.cppm
		src/core/core.cppm
		#src/core/timer.cppm MAYBE LATER
		src/core/window_context.cppm
		src/core/input_manager.cppm
		src/graphics/renderer/shader_storage_buffer.cppm
		src/graphics/debug_drawer.cppm
		src/graphics/shader.cppm
		src/game/player.cppm
		src/ui/ui.cppm
)


# ==== Compiler & Linker Logic ====
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(${PROJECT_NAME} PRIVATE "-fuse-ld=mold")
else()
    target_link_options(${PROJECT_NAME} PRIVATE "-fuse-ld=mold" "-static-libgcc" "-static-libstdc++")
endif()



# ==== Tracy Integration ====
if(ENABLE_TRACY)
    add_library(tracy_client STATIC
        ${tracy_SOURCE_DIR}/public/TracyClient.cpp
    )

    target_include_directories(tracy_client PUBLIC
        ${tracy_SOURCE_DIR}/public
    )

    target_compile_definitions(tracy_client PUBLIC
        TRACY_ENABLE
        TRACY_GPU_CONTEXT
        TRACY_CALLSTACK
        $<$<BOOL:${CMAKE_INTERPROCEDURAL_OPTIMIZATION}>:TRACY_LTO>
    )

    target_compile_options(tracy_client PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-fno-omit-frame-pointer>
        $<$<CXX_COMPILER_ID:Clang>:-fno-omit-frame-pointer>
        $<$<CXX_COMPILER_ID:MSVC>:/Oy->
    )

    if(WIN32)
        target_link_libraries(tracy_client PRIVATE ws2_32 dbghelp)
    endif()
endif()


# ==== Source Files ====


target_include_directories(${PROJECT_NAME}
	PRIVATE
	${PROJECT_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/include/external/vendor
	${IMGUI_INCLUDE_DIRS}
	#SYSTEM PRIVATE ${IMGUI_INCLUDE_DIRS}
)

# ==== Compiler Flags ====
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    PROJECT_NAME="${PROJECT_NAME}"
    PROJECT_VERSION="v${PROJECT_FULL_VERSION}"
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_VERSION_SUFFIX="${PROJECT_VERSION_SUFFIX}"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:
        -D_GLIBCXX_ASSERTIONS
        -fexceptions
        -Wall
        -Wextra
        -pedantic
        -Wuninitialized
		-g
        -ggdb
		-flto
    >
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:
        -O3
        -s
        -flto
        -fomit-frame-pointer
        -funsafe-math-optimizations
        -fno-math-errno
    >
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:
        /W4
        /Zi
        /EHsc
        /MDd
        /GL
    >
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:
        /O2
        /GL
        /MD
        /fp:fast
    >
)

# ==== Platform-Specific Libraries ====
if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
elseif(UNIX AND NOT APPLE)
		find_package(PkgConfig REQUIRED)

		# Try to find Wayland first
		pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor wayland-egl)
		if(WAYLAND_FOUND)
			message(STATUS "Found Wayland: ${WAYLAND_INCLUDE_DIRS} / ${WAYLAND_LIBRARIES}")
			add_library(Wayland::Client INTERFACE IMPORTED)
			set_target_properties(Wayland::Client PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_INCLUDE_DIRS}"
				INTERFACE_LINK_LIBRARIES "${WAYLAND_LIBRARIES}"
			)
			target_link_libraries(${PROJECT_NAME} PRIVATE Wayland::Client)
		else()
			message(STATUS "Wayland not found, falling back to X11")
			find_package(X11 REQUIRED)
			target_link_libraries(${PROJECT_NAME} PRIVATE X11::X11)
		endif()
endif()

# ==== Link Everything ====
target_link_libraries(${PROJECT_NAME} PRIVATE
	PUBLIC 
	$<$<BOOL:${ENABLE_TRACY}>:tracy_client>
	c++
	c++abi
    glfw
    freetype
    RmlUi::Core
    RmlUi::Debugger
    imgui
	FastNoise2
	game_c
)
