cmake_minimum_required(VERSION 3.15...4.0.1)
cmake_policy(VERSION 3.25)  # Set this to highest version you've tested with

# Project Name
project("opengl" VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
include(FetchContent)


# GLEW (CMake-friendly fork)
FetchContent_Declare(
  glew
  GIT_REPOSITORY https://github.com/Perlmint/glew-cmake.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "Build static libs" FORCE)
set(GLEW_USE_STATIC_LIBS ON)
set(GLEW_BUILD_STATIC ON)
set(GLEW_BUILD_SHARED OFF)
set(GLEW_BUILD_EXAMPLES OFF)
set(GLEW_BUILD_TESTS OFF)
set(GLEW_BUILD_DOCS OFF)
FetchContent_MakeAvailable(glew)
#message(STATUS "Targets from GLEW dir: ${GLEW_SUBDIR}")
#message(STATUS "GLEW (CMake version) fetched from: ${glew_SOURCE_DIR}")
#message(STATUS "GLEW build dir: ${glew_BINARY_DIR}")

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        master     # assumes they're using master for development
  GIT_SHALLOW    TRUE     # faster fetch
  GIT_PROGRESS   TRUE
)
set(GLFW_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(glm)


# FreeType setup
FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://github.com/freetype/freetype.git
  GIT_TAG master
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
option(FT_DISABLE_ZLIB "Disable use of system zlib and use internal zlib library instead." ON)
option(FT_DISABLE_BZIP2 "Disable support of bzip2 compressed fonts." ON)
option(FT_DISABLE_PNG "Disable support of PNG compressed OpenType embedded bitmaps." ON)
option(FT_DISABLE_HARFBUZZ "Disable HarfBuzz (used for improving auto-hinting of OpenType fonts)." ON)
option(FT_DISABLE_BROTLI "Disable support of compressed WOFF2 fonts." ON)
FetchContent_MakeAvailable(freetype)
#message(STATUS "Freetype_ROOT: ${freetype_ROOT_DIR}")
#message(STATUS "freetype_BINARY_DIR: ${freetype_BINARY_DIR}")
#message(STATUS "FreeType fetched from: ${freetype_SOURCE_DIR}")

# Ensure FreeType is found by RmlUi
set(Freetype_ROOT ${freetype_BINARY_DIR} CACHE INTERNAL "FreeType build directory")
set(FREETYPE_LIBRARY ${freetype_BINARY_DIR}/libfreetype.a CACHE INTERNAL "FreeType library")
set(FREETYPE_INCLUDE_DIRS ${freetype_SOURCE_DIR}/include CACHE INTERNAL "FreeType include directories")
add_library(Freetype::Freetype ALIAS freetype)
find_package(Freetype REQUIRED)
message(STATUS "Freetype found: ${FREETYPE_FOUND}")
message(STATUS "Freetype libraries: ${FREETYPE_LIBRARIES}")
message(STATUS "Freetype include dirs: ${FREETYPE_INCLUDE_DIRS}")
if(NOT FREETYPE_FOUND)
  message(FATAL_ERROR "Freetype not found by find_package(Freetype)")
endif()

# RmlUi setup
FetchContent_Declare(
  RmlUi
  GIT_REPOSITORY https://github.com/mikke89/RmlUi.git
  GIT_TAG master
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
)
option(RMLUI_ENABLE_DEBUGGER "Enable RmlUi Debugger" ON)
set(RMLUI_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RMLUI_BUILD_LIBRMLDEBUGGER ON CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "Force freetype font engine" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Force static RmlUi library" FORCE)
set(RMLUI_SHELL OFF CACHE BOOL "Force disabling shell" FORCE)
set(CMAKE_MESSAGE_LOG_LEVEL TRACE CACHE STRING "Set log level to TRACE for verbose output" FORCE)
set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "Enable verbose output" FORCE)
FetchContent_MakeAvailable(RmlUi)
message(STATUS "RmlUi fetched from: ${RmlUi_SOURCE_DIR}")
if(NOT TARGET RmlUi::Core)
  message(FATAL_ERROR "RmlUi::Core target not found after FetchContent_MakeAvailable(RmlUi). Check _deps/rmlui-src/Source/CMakeLists.txt or CMake/Dependencies.cmake for errors.")
endif()
message(STATUS "RmlUi::RmlCore target exists: $<TARGET_EXISTS:RmlUi::RmlCore>")


# Then manually add a pre-release label
set(PROJECT_VERSION_SUFFIX "-pre-alpha")
set(PROJECT_FULL_VERSION "${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX}")
set(EXECUTABLE_OUTPUT_NAME "${PROJECT_NAME}-v${PROJECT_FULL_VERSION}")

# Set C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Generate compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



# Set the default build type to Release if not set explicitly
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()


# Find Required Packages
find_package(OpenGL REQUIRED)

if (NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
endif()

# Output Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/release)

# Source Files
file(GLOB_RECURSE PROJECT_SOURCE_FILES src/*.cpp include/external/imgui/*.cpp)


# Executable
add_executable(${PROJECT_NAME} main.cpp ${PROJECT_SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${EXECUTABLE_OUTPUT_NAME}
)

# Compiler Flags
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    GLEW_STATIC
    PROJECT_NAME="${PROJECT_NAME}"
    PROJECT_VERSION="v${PROJECT_FULL_VERSION}"
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_VERSION_SUFFIX="${PROJECT_VERSION_SUFFIX}"
)

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
	-g 
	-fexceptions 
	-Wall 
	-Wextra 
	-pedantic  
	-Wuninitialized 
	-ggdb 
    > # use -Werror if u want warning treated as errors!!!! && use -Wall to
    $<$<CONFIG:Release>:
	-O3 
	-s 
	-flto 
	-fomit-frame-pointer 
	-funsafe-math-optimizations 
	-fno-math-errno 
    > # Beware of -match=native as it locks the build to the CPU type it's beign built in
)

# Linking
if (WIN32)
    target_link_libraries(${PROJECT_NAME} 
	PUBLIC OpenGL::GL 
	PRIVATE libglew_static glfw glm::glm freetype RmlUi::Core RmlUi::Debugger opengl32
    )
elseif (UNIX)
    target_link_libraries(${PROJECT_NAME} 
	PUBLIC OpenGL::GL 
	PRIVATE libglew_static glfw glm::glm freetype RmlUi::Core RmlUi::Debugger GL X11 Xext
    )
endif()

# Include Paths
target_include_directories(${PROJECT_NAME} PRIVATE
    ${glew_SOURCE_DIR}/include
    ${rmlui_SOURCE_DIR}/Include/RmlUi
    ${rmlui_SOURCE_DIR}/Backends
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/UI
    ${PROJECT_SOURCE_DIR}/include/Chunk
    ${PROJECT_SOURCE_DIR}/include/Player
    ${PROJECT_SOURCE_DIR}/include/Player/player_states
    ${PROJECT_SOURCE_DIR}/include/Player/player_modes
    ${PROJECT_SOURCE_DIR}/include/external
    ${PROJECT_SOURCE_DIR}/include/external/imgui
    ${PROJECT_SOURCE_DIR}/include/external/imgui/backends
)
target_sources(${PROJECT_NAME} PRIVATE
    ${rmlui_SOURCE_DIR}/Include/RmlUi/Core.h
)



# === Post-build: Copy Shaders and Assets ===

# Source directories (relative to your project root)
set(SHADERS_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}")
endif()
# Define custom command to copy shaders
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SHADERS_DIR}" "${OUTPUT_DIR}/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_DIR}" "${OUTPUT_DIR}/assets"
    COMMENT "Copying shaders and assets to output directory..."
)

