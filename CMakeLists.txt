cmake_minimum_required(VERSION 3.30...4.0.1)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1)


set(CMAKE_CXX_COMPILER_LAUNCHER sccache)
project("game" VERSION 0.2.0 LANGUAGES C CXX)


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

# ==== Source Files ====
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS src/*.cpp)


set_source_files_properties(${GLAD_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
add_library(game_c STATIC ${GLAD_SOURCES})
target_include_directories(game_c PRIVATE
    $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/include/external/glad-debug>
    $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/include/external/glad-release>
)
disable_debug_symbols(glfw)
disable_debug_symbols(freetype)
disable_debug_symbols(RmlCore)
disable_debug_symbols(RmlDebugger)

# ==== Main Executable ====
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${EXECUTABLE_OUTPUT_NAME}
    UNITY_BUILD ${ENABLE_UNITY_BUILD}
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    LINKER_LANGUAGE CXX
)
# ==== Include compiler flags ====
include(cmake/compiler_flags.cmake)


# ==== Include Paths ====
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/external
    $<$<CONFIG:Debug>:${PROJECT_SOURCE_DIR}/include/external/glad-debug/>
    $<$<CONFIG:Release>:${PROJECT_SOURCE_DIR}/include/external/glad-release/>
    SYSTEM PRIVATE ${IMGUI_INCLUDE_DIRS}
    ${glm_SOURCE_DIR}
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE_DIR}/include/pch_wrapper.hpp
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
target_link_libraries(${PROJECT_NAME} PRIVATE ${COMMON_LIBS} ${PLATFORM_LIBS})

# ==== Tracy Integration ====
if(ENABLE_TRACY)
    target_sources(${PROJECT_NAME} PRIVATE
        ${tracy_SOURCE_DIR}/public/TracyClient.cpp
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${tracy_SOURCE_DIR}/public
    )

    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:-fno-omit-frame-pointer>
        $<$<CXX_COMPILER_ID:Clang>:-fno-omit-frame-pointer>
        $<$<CXX_COMPILER_ID:MSVC>:/Oy->
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE
        TRACY_ENABLE
        TRACY_GPU_CONTEXT
        TRACY_CALLSTACK
        $<$<BOOL:${CMAKE_INTERPROCEDURAL_OPTIMIZATION}>:TRACY_LTO>
    )

    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 dbghelp)
    endif()
endif()










# ==== Post-build asset copy ====
set(SHADERS_DIR "${CMAKE_SOURCE_DIR}/src/graphics/shaders")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADERS_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_DIR}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    COMMENT "Copying shaders and assets to executable directory"
)
