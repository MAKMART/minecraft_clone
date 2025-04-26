cmake_minimum_required(VERSION 3.10)

# Project Name
project("opengl" CXX)

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

# Ensure CMake Prefix Path is Set
set(CMAKE_PREFIX_PATH "C:/msys64/mingw64/include")

# Find Required Packages
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Ensure the packages were found
if (NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
endif()

if (NOT GLEW_FOUND)
    message(FATAL_ERROR "GLEW package not found!")
endif()

if (NOT glfw3_FOUND)
    message(FATAL_ERROR "GLFW package not found!")
endif()

if (NOT glm_FOUND)
    message(FATAL_ERROR "GLM package not found!")
endif()

# Output Directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Maybe FIX before with nvim
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/build/Debug)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/build/Release)

# Source Files
file(GLOB_RECURSE PROJECT_SOURCE_FILES src/*.cpp include/external/imgui/*.cpp)

# Executable
add_executable(${PROJECT_NAME} main.cpp ${PROJECT_SOURCE_FILES})

# Compiler Flags
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

target_compile_options(${PROJECT_NAME} PRIVATE # use -Werror if u want warning treated as errors!!!! && use -Wall to 
    $<$<CONFIG:Debug>:-g -fexceptions -Wall -Wextra -pedantic  -Wuninitialized -ggdb >
    $<$<CONFIG:Release>:-O3 -Wall -Wextra -pedantic  -flto -march=native>
)

# Linking
target_link_libraries(${PROJECT_NAME} 
    PUBLIC OpenGL::GL 
    PRIVATE GLEW::GLEW glfw glm::glm
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
