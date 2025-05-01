cmake_minimum_required(VERSION 3.15)

# Project Name
project("opengl" VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

# GLEW
FetchContent_Declare(
  glew
  GIT_REPOSITORY https://github.com/nigels-com/glew.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(glew)

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.8
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


# Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Maybe FIX before with nvim
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/release)

# Source Files
file(GLOB_RECURSE PROJECT_SOURCE_FILES src/*.cpp include/external/imgui/*.cpp)

find_package(OpenGL REQUIRED)

if (NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
endif()

# Executable
add_executable(${PROJECT_NAME} main.cpp ${PROJECT_SOURCE_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${EXECUTABLE_OUTPUT_NAME}
)

# Compiler Flags
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    PROJECT_NAME="${PROJECT_NAME}"
    PROJECT_VERSION="${PROJECT_FULL_VERSION}"
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
    PRIVATE GLEW::glew_s glfw glm::glm
)

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
