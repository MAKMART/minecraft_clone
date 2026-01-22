# ==== Build Options ====
option(ENABLE_TRACY "Enable Tracy profiler (Debug only)" ON)
set(ENABLE_DOCS "Enable building documentation with Doxygen" OFF)
option(ENABLE_UNITY_BUILD "Enable unity build" ON)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build everything statically" FORCE)
if(MSVC)
    # MSVC usually handles runtime linking automatically
else()
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -static-libgcc -static-libstdc++")
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_POLICY_WARNING_CMPNNNN OFF) # for specific CMP warnings
set(CMAKE_MESSAGE_LOG_LEVEL STATUS)

# ==== C++ Standard ====
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==== Versioning ====
set(PROJECT_VERSION_SUFFIX "-alpha")
set(PROJECT_FULL_VERSION "${PROJECT_VERSION}${PROJECT_VERSION_SUFFIX}")
set(EXECUTABLE_OUTPUT_NAME "${PROJECT_NAME}-v${PROJECT_FULL_VERSION}")

# ==== Build Type Defaults ====
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

# ==== Linking ====
set(CMAKE_EXE_LINKER_FLAGS "-fuse-ld=mold")

# ==== IPO for release ====
if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
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


# ==== GLAD ====
set(GLAD_SOURCES
	$<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/src/external/glad_debug.c>
	$<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/src/external/glad_release.c>
)

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

# ==== RmlUi ====
set(RMLUI_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RMLUI_BUILD_LIBRMLDEBUGGER ON CACHE BOOL "" FORCE)
set(RMLUI_FONT_ENGINE "freetype" CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(RMLUI_SHELL OFF CACHE BOOL "" FORCE)

# ==== Tracy (optional) ====
if(ENABLE_TRACY)
    set(TRACY_SKIP_GIT_UPDATE ON CACHE BOOL "" FORCE)
    set(TRACY_ENABLE_VERBOSE OFF)
endif()

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
