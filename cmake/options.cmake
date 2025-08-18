# ==== Build Options ====
option(ENABLE_TRACY "Enable Tracy profiler (Debug only)" ON)
option(ENABLE_DOCS "Enable building documentation with Doxygen" OFF)
option(ENABLE_UNITY_BUILD "Enable unity build" ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build everything statically" FORCE)
string(APPEND CMAKE_EXE_LINKER_FLAGS " -static-libgcc -static-libstdc++")
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)


# ==== C++ Standard ====
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==== Versioning ====
set(PROJECT_VERSION_SUFFIX "-pre-alpha")
set(PROJECT_FULL_VERSION "${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX}")
set(EXECUTABLE_OUTPUT_NAME "${PROJECT_NAME}-v${PROJECT_FULL_VERSION}")

# ==== Build Type Defaults ====
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# ==== IPO for release ====
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# ==== Tracy warnings ====
if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ENABLE_TRACY)
    message(WARNING "⚠️ Tracy is enabled in Release build — this may reduce performance.")
endif()

# ==== Compile Commands for clangd ====
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==== Output Directories ====
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/release)
