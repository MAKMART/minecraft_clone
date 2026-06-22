add_executable(${PROJECT_NAME})
# set(OUTPUT_BASE_DIR "${CMAKE_BINARY_DIR}/bin")
set_target_properties(${PROJECT_NAME}
	PROPERTIES
  CXX_STANDARD 26
  CXX_STANDARD_REQUIRED ON
	OUTPUT_NAME "${PROJECT_NAME}-v${PROJECT_FULL_VERSION}"
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}"
)
get_target_property(std ${PROJECT_NAME} CXX_STANDARD)
get_target_property(extensions ${PROJECT_NAME} CXX_EXTENSIONS)
get_target_property(out_name ${PROJECT_NAME} OUTPUT_NAME)
get_target_property(out_dir ${PROJECT_NAME} RUNTIME_OUTPUT_DIRECTORY)
message(STATUS "-----------------------------------------------")
message(STATUS "GAME             : ${out_name}")
message(STATUS "C++ Standard     : ${std}")
message(STATUS "C++ Extensions   : ${extensions}")
message(STATUS "Output directory : ${out_dir}")


# ==== IMGUI ====
add_library(imgui STATIC)
target_sources(imgui
  PRIVATE
  vendor/imgui/imgui.cpp
  vendor/imgui/imgui_draw.cpp
  vendor/imgui/imgui_tables.cpp
  vendor/imgui/imgui_widgets.cpp
  vendor/imgui/backends/imgui_impl_glfw.cpp
  vendor/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui
  PRIVATE
  ${PROJECT_SOURCE_DIR}/vendor/imgui
  ${PROJECT_SOURCE_DIR}/vendor/imgui/backends
  ${glfw_SOURCE_DIR}/include
)
target_link_libraries(imgui PRIVATE glfw)


target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    PROJECT_NAME="${PROJECT_NAME}"
    PROJECT_VERSION="v${PROJECT_FULL_VERSION}"
    PROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    PROJECT_VERSION_SUFFIX="${PROJECT_VERSION_SUFFIX}"
)

target_compile_options(${PROJECT_NAME} PRIVATE
        # -fno-implicit-modules
        # -fno-implicit-module-maps
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:
        -D_GLIBCXX_ASSERTIONS
        -fexceptions
        -fsanitize=address
        -Wall
        -Wextra
        -pedantic
        -Wuninitialized
        -g
        -ggdb
        -flto
    >
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Release>>:
        -O3
        -s
        -flto
        -fomit-frame-pointer
        -funsafe-math-optimizations
        -fno-math-errno
    >
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:
        /W4
        /Zi
        /EHsc
        /MDd
        /GL
    >
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:
        /O2
        /GL
        /MD
        /fp:fast
    >
)
# ==== Source Files ====
target_sources(${PROJECT_NAME}
  PRIVATE
  main.cpp
  core/core.cpp
  core/timer.cpp
  chunk/chunk_manager.cpp
  app/ecs/systems.cpp
  app/app.cpp
  $<$<BOOL:${ENABLE_TRACY}>:src/tracy_overrides.cpp>
  PRIVATE
  FILE_SET CXX_MODULES
  FILES
  app/app.cppm
  app/ecs/components.cppm
  app/ecs/systems.cppm
  app/player.cppm
  app/game_state.cppm
  app/game_factories.cppm
  chunk/mesher.cppm # Custom mesher.cppm implementatino in game
  chunk/noise.cppm
  chunk/chunk_manager.cppm
  chunk/chunk.cppm
  chunk/chunk_renderer.cppm
  core/raycast.cppm
  core/core.cppm
  core/timer.cppm
  debug.cppm
)

# ==== Compiler & Linker Logic ====
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(${PROJECT_NAME} PRIVATE "-fuse-ld=mold")
else()
    target_link_options(${PROJECT_NAME} PRIVATE "-fuse-ld=mold" "-static-libstdc++")
endif()

target_include_directories(${PROJECT_NAME}
	PRIVATE
	${IMGUI_INCLUDE_DIRS}
)

if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE OpenGL::GL)
elseif(UNIX AND NOT APPLE)
		find_package(PkgConfig REQUIRED)

		# Try to find Wayland first
		pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-cursor wayland-egl)
		if(WAYLAND_FOUND)
			# message(STATUS "Found Wayland: ${WAYLAND_INCLUDE_DIRS} / ${WAYLAND_LIBRARIES}")
			add_library(Wayland::Client INTERFACE IMPORTED)
			set_target_properties(Wayland::Client PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_INCLUDE_DIRS}"
				INTERFACE_LINK_LIBRARIES "${WAYLAND_LIBRARIES}"
			)
			target_link_libraries(${PROJECT_NAME} PRIVATE Wayland::Client)
		else()
			message(STATUS "Wayland not found, falling back to X11")
			find_package(X11 REQUIRED)
			target_link_libraries(${PROJECT_NAME} PRIVATE X11::X11)
		endif()
endif()

# ==== Link Everything ====
find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
endif()

target_link_libraries(${PROJECT_NAME}
  PRIVATE
  $<$<BOOL:${ENABLE_TRACY}>:tracy_client>
  imgui
  FastNoise2
  binary-greedy-meshing
  engine
)

# ==== FastNoise2 ====
function(configure_fast_noise)
  set(FASTNOISE2_NOISETOOL OFF CACHE BOOL "" FORCE)
  set(FASTNOISE2_TESTS OFF CACHE BOOL "" FORCE)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FASTNOISE2_SIMD_FEATURES "AVX2" CACHE STRING "SIMD level for FastNoise2 in Debug")
  else()
    # Full SIMD dispatch in Release builds
    set(FASTNOISE2_SIMD_FEATURES "SSE2;SSE41;AVX2;AVX512" CACHE STRING "SIMD levels for FastNoise2 in Release")
  endif()
  set(FASTSIMD_FEATURE_SETS "${FASTNOISE2_SIMD_FEATURES}")
  add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/fast-noise-2 ${CMAKE_BINARY_DIR}/fastnoise)
endfunction()
configure_fast_noise()


# binary-greedy-meshing library (remember to cite the authors when publishing)
function(configure_meshing)
add_library(binary-greedy-meshing STATIC)
set(BGM_ROOT ${PROJECT_SOURCE_DIR}/vendor/binary-greedy-meshing)
target_sources(binary-greedy-meshing
  PUBLIC
  FILE_SET CXX_MODULES
  BASE_DIRS ${BGM_ROOT}
  FILES
  # Use custom mesher.cppm in game
  # ${BGM_ROOT}/mesher.cppm
  # vendor/binary-greedy-meshing/rendering/chunk_renderer.cppm
  ${BGM_ROOT}/misc/noise.cppm
  ${BGM_ROOT}/misc/utility.cppm
)
set_target_properties(binary-greedy-meshing
	PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
)

target_include_directories(binary-greedy-meshing
  PRIVATE
  vendor/binary-greedy-meshing/
  vendor/glad
  vendor/fast-noise-2/include
)

target_link_libraries(binary-greedy-meshing PUBLIC FastNoise2)
endfunction()
configure_meshing()

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/shaders" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_SOURCE_DIR}/compile_commands.json
)
