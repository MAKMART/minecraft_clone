cmake_minimum_required(VERSION 3.30...4.0.1)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)

set(CMAKE_CXX_COMPILER_LAUNCHER sccache)
project("game" VERSION 1.0.0 LANGUAGES C CXX)

# Include configuration
include(cmake/options.cmake)
include(cmake/dependencies.cmake)

message(STATUS "Build Type       : ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard     : C++${CMAKE_CXX_STANDARD}")
message(STATUS "Unity Build      : ${ENABLE_UNITY_BUILD}")
message(STATUS "Tracy Profiler   : ${ENABLE_TRACY}")
message(STATUS "Documentation    : ${ENABLE_DOCS}")
message(STATUS "Executable Name  : ${EXECUTABLE_OUTPUT_NAME}")

find_package(OpenGL REQUIRED)

if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
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
file(GLOB_RECURSE GAME_CORE_SOURCES
    src/core/*.cpp src/graphics/*.cpp src/chunk/*.cpp src/ui/*.cpp src/game/*.cpp
)

add_library(game_core STATIC ${GAME_CORE_SOURCES})
target_precompile_headers(game_core PRIVATE
	${PROJECT_SOURCE_DIR}/include/pch_wrapper.hpp
)
set_source_files_properties(src/main.cpp PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
target_include_directories(game_core 
	PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/external/vendor
    $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/include/external/glad-debug/>
    $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/include/external/glad-release/>
	${IMGUI_INCLUDE_DIRS}
	#SYSTEM PRIVATE ${IMGUI_INCLUDE_DIRS}
    ${glm_SOURCE_DIR}
)
target_compile_definitions(game_core
    PUBLIC
        $<$<BOOL:${ENABLE_TRACY}>:TRACY_ENABLE>
)

set_target_properties(game_core PROPERTIES
    UNITY_BUILD ${ENABLE_UNITY_BUILD}
)
# ==== Common Libraries ====
set(COMMON_LIBS
    glfw
    freetype
    RmlUi::Core
    RmlUi::Debugger
    imgui
    game_c
)
target_link_libraries(game_core 
	PUBLIC 
	$<$<BOOL:${ENABLE_TRACY}>:tracy_client>
	${COMMON_LIBS}
	FastNoise2
)


set_source_files_properties(${GLAD_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
add_library(game_c STATIC ${GLAD_SOURCES})
target_include_directories(game_c PRIVATE
    $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/include/external/glad-debug>
    $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/include/external/glad-release>
)
disable_debug_symbols(glfw)
disable_debug_symbols(freetype)

# ==== Main Executable ====
add_executable(${PROJECT_NAME} src/main.cpp $<$<BOOL:${ENABLE_TRACY}>:src/tracy_overrides.cpp>)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME "${EXECUTABLE_OUTPUT_NAME}"
)

include(cmake/compiler_flags.cmake)

# ==== Platform-Specific Libraries ====
if(WIN32)
    set(PLATFORM_LIBS opengl32)
elseif(UNIX)
    if(DEFINED ENV{WAYLAND_DISPLAY})
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(EGL REQUIRED egl)

        target_include_directories(${PROJECT_NAME} PRIVATE ${EGL_INCLUDE_DIRS})
        target_link_directories(${PROJECT_NAME} PRIVATE ${EGL_LIBRARY_DIRS})

        set(PLATFORM_LIBS ${EGL_LIBRARIES})
    else()
        set(PLATFORM_LIBS GL X11 Xext)
    endif()
endif()

# ==== Link Everything ====
target_link_libraries(${PROJECT_NAME} PUBLIC game_core ${PLATFORM_LIBS})



# ==== Post-build asset copy ====
set(SHADERS_DIR "${CMAKE_SOURCE_DIR}/src/graphics/shaders")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADERS_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying shaders and assets to executable directory"
)
