cmake_minimum_required(VERSION 4.4.0)
if(NOT CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "You must choose the type of build")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")

# C++ standard library modules
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
# set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
set(CMAKE_CXX_STDLIB_MODULES_JSON
"/usr/lib/libc++.modules.json")
# message(STATUS "CXX compiler: ${CMAKE_CXX_COMPILER}")
# message(STATUS "Modules std: ${CMAKE_CXX_MODULE_STD}")
project("game" VERSION 1.0.1 LANGUAGES C CXX)

set(PROJECT_FULL_VERSION "${PROJECT_VERSION}-alpha")

set(CMAKE_CXX_STANDARD 26)          # or at least 23
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Compiler extensions disabled globally message(STATUS "[TOOLCHAIN] Loaded")
# ==== Build Options ====
option(ENABLE_UNITY_BUILD "Enable unity build" OFF)
option(ENABLE_TRACY "Enable Tracy profiler" OFF)
option(ENABLE_TESTS "Build engine tests" OFF)
option(ENABLE_GAME "Build game" OFF)

message(STATUS "-----------------------------------------------")
message(STATUS "Project Version  : ${PROJECT_FULL_VERSION}")
message(STATUS "Build Type       : ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler ID      : ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler Version : ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Unity Build      : ${ENABLE_UNITY_BUILD}")
message(STATUS "Tracy Profiler   : ${ENABLE_TRACY}")
message(STATUS "Documentation    : ${ENABLE_DOCS}")

if(CMAKE_BUILD_TYPE STREQUAL "Release" AND ENABLE_TRACY)
    message(WARNING "Tracy is enabled in Release build — this may reduce performance.")
endif()

# ==== Tracy (optional) ====
# function(configure_tracy)
#   set(TRACY_SKIP_GIT_UPDATE ON CACHE BOOL "" FORCE)
#   set(TRACY_ENABLE_VERBOSE OFF)
#   include(FetchContent)
#   FetchContent_Declare(tracy
#     GIT_REPOSITORY https://github.com/wolfpld/tracy.git
#     GIT_TAG master
#     GIT_SHALLOW TRUE
#     GIT_PROGRESS TRUE
#   )
#   FetchContent_MakeAvailable(tracy)
#
#   add_library(tracy_client STATIC
#     ${tracy_SOURCE_DIR}/public/TracyClient.cpp
#   )
#
#   target_include_directories(tracy_client PUBLIC
#     ${tracy_SOURCE_DIR}/public
#   )
#
#   target_compile_definitions(tracy_client PUBLIC
#     TRACY_ENABLE
#     TRACY_GPU_CONTEXT
#     TRACY_CALLSTACK
#     $<$<BOOL:${CMAKE_INTERPROCEDURAL_OPTIMIZATION}>:TRACY_LTO>
#   )
#
#   target_compile_options(tracy_client PRIVATE
#     $<$<CXX_COMPILER_ID:GNU>:-fno-omit-frame-pointer>
#     $<$<CXX_COMPILER_ID:Clang>:-fno-omit-frame-pointer>
#     $<$<CXX_COMPILER_ID:MSVC>:/Oy->
#   )
#
#   if(WIN32)
#     target_link_libraries(tracy_client PRIVATE ws2_32 dbghelp)
#   endif()
# endfunction()

# if(ENABLE_TRACY)
#   configure_tracy()
# endif()


add_subdirectory(engine)

if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(engine/tests)
endif()

if(ENABLE_GAME)
    # add_subdirectory(game)
    add_subdirectory(engine/test_app)
endif()
