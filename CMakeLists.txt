cmake_minimum_required(VERSION 3.15...4.0.1)
cmake_policy(VERSION 3.25)  # Set this to highest version you've tested with

# Project Name
project("opengl" VERSION 0.1.0 LANGUAGES CXX)

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
FetchContent_MakeAvailable(glew)
message(STATUS "GLEW (CMake version) fetched from: ${glew_SOURCE_DIR}")

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        master     # assumes they're using main for development
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

# Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Maybe FIX before with nvim
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

target_compile_options(${PROJECT_NAME} PRIVATE # use -Werror if u want warning treated as errors!!!! && use -Wall to 
    $<$<CONFIG:Debug>:-g -fexceptions -Wall -Wextra -pedantic  -Wuninitialized -ggdb >
    $<$<CONFIG:Release>:-O3 -Wall -Wextra -pedantic  -flto -march=native>
)

# Linking
target_link_libraries(${PROJECT_NAME} 
    PUBLIC OpenGL::GL 
    PRIVATE ${glew_BINARY_DIR}/lib/libglewd.a glfw glm::glm
    opengl32
)
message(STATUS "GLEW build dir: ${glew_BINARY_DIR}")
target_link_directories(${PROJECT_NAME} PRIVATE ${glew_BINARY_DIR}/lib)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLEW_STATIC)
# Include Paths
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/Chunk
    ${PROJECT_SOURCE_DIR}/include/Player
    ${PROJECT_SOURCE_DIR}/include/Player/player_states
    ${PROJECT_SOURCE_DIR}/include/Player/player_modes
    ${PROJECT_SOURCE_DIR}/include/external
    ${PROJECT_SOURCE_DIR}/include/external/imgui
    ${PROJECT_SOURCE_DIR}/include/external/imgui/backends
)
