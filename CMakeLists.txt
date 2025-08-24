cmake_minimum_required(VERSION 3.30...4.0.1)


if(POLICY CMP0126)
    cmake_policy(SET CMP0126 NEW)
endif()

project("game" VERSION 0.2.0 LANGUAGES CXX)


# Include configuration
include(cmake/options.cmake)
include(cmake/dependencies.cmake)

if(ENABLE_TRACY AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "✅ Tracy is ENABLED")
else()
    message(STATUS "🚫 Tracy is DISABLED")
endif()

if(ENABLE_UNITY_BUILD)
    message(STATUS "✅ Unity build is ENABLED")
else()
    message(STATUS "🚫 Unity build is DISABLED")
endif()

find_package(OpenGL REQUIRED)

if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
endif()

# ==== Source Files ====
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS src/*.cpp)

# ==== Main Executable ====
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${EXECUTABLE_OUTPUT_NAME}
    UNITY_BUILD ${ENABLE_UNITY_BUILD}
)

# ==== Include compiler flags ====
include(cmake/compiler_flags.cmake)


# ==== Include Paths ====
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/external
    ${IMGUI_INCLUDE_DIRS}
    ${glew_SOURCE_DIR}/include
    ${glew_BINARY_DIR}/include
    ${glm_SOURCE_DIR}
)


# ==== Linking ====
if (WIN32)
    target_link_libraries(${PROJECT_NAME} 
        PUBLIC OpenGL::GL libglew_static
	PRIVATE glfw freetype RmlUi::Core RmlUi::Debugger opengl32 imgui
    )
elseif (UNIX)
    if (DEFINED ENV{WAYLAND_DISPLAY})
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(EGL REQUIRED egl)
        target_include_directories(${PROJECT_NAME} PRIVATE ${EGL_INCLUDE_DIRS})
        target_link_directories(${PROJECT_NAME} PRIVATE ${EGL_LIBRARY_DIRS})

        target_link_libraries(${PROJECT_NAME} 
            PUBLIC OpenGL::GL libglew_static
            PRIVATE glfw freetype RmlUi::Core RmlUi::Debugger ${EGL_LIBRARIES} imgui
        )
    else()
        target_link_libraries(${PROJECT_NAME} 
            PUBLIC OpenGL::GL libglew_static
            PRIVATE glfw freetype RmlUi::Core RmlUi::Debugger GL X11 Xext imgui
        )
    endif()
endif()

# ==== Tracy Integration ====
if(ENABLE_TRACY)
    if (WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 dbghelp)
    endif()
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
