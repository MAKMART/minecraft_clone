add_library(engine STATIC)
set_target_properties(engine
	PROPERTIES
  CXX_STANDARD 26
  CXX_STANDARD_REQUIRED ON
  OUTPUT_NAME "engine"
	# RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}"
  # ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_BASE_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
# Make sure we have needed includes available
target_include_directories(engine
  # Why tf does PUBLIC work, I don't want to make them public :(
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
)
# ==== Compiler & Linker Logic ====
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_link_options(engine PRIVATE "-fuse-ld=mold")
else()
  target_link_options(engine PRIVATE "-fuse-ld=mold" "-static-libstdc++")
endif()

get_target_property(std engine CXX_STANDARD)
get_target_property(extensions engine CXX_EXTENSIONS)
get_target_property(out_name engine OUTPUT_NAME)
get_target_property(out_dir engine ARCHIVE_OUTPUT_DIRECTORY)
message(STATUS "-----------------------------------------------")
message(STATUS "ENGINE           : ${out_name}")
message(STATUS "C++ Standard     : ${std}")
message(STATUS "C++ Extensions   : ${extensions}")
message(STATUS "Output directory : ${out_dir}")

find_package(OpenGL REQUIRED)
if(NOT OpenGL_FOUND)
    message(FATAL_ERROR "OpenGL package not found!")
endif()

target_compile_definitions(engine PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    # To make the glm module include all the things in the glm::* gtc::* and gtx::* namespaces
    GLM_GTC_INLINE_NAMESPACE
    GLM_EXT_INLINE_NAMESPACE
    GLM_GTX_INLINE_NAMESPACE
)
target_compile_options(engine PRIVATE
        # -fno-implicit-modules
        # -fno-implicit-module-maps
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:Debug>>:
        -D_GLIBCXX_ASSERTIONS
        -fexceptions
        # -freflection
        # If you want to use the address sanitizer then you must also put it in the test_app and tests and game
        # -fsanitize=address
        -Wall
        -Wextra
        -pedantic
        -Wuninitialized
        -g
        -ggdb
        # -flto
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

target_sources(engine
  PUBLIC
  FILE_SET public_api
  TYPE CXX_MODULES
  BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
  FILES
  # .cppm modules here
  # core
  core/core.cppm
  core/types.cppm
  core/logger.cppm
  core/events.cppm
  # runtime
  runtime/runtime.cppm
  # runtime/frame_context.cppm
  # ecs
  ecs/ecs.cppm
  ecs/entity.cppm
  # math
  vendor/glm/glm/glm.cppm
  math/math.cppm
  math/types.cppm
  math/aabb.cppm
  # platform
  platform/platform.cppm
  platform/input_state.cppm
  platform/input_action_map.cppm
  platform/window.cppm
  # renderer
  renderer/renderer.cppm
  # renderer/debug_drawer.cppm
  #   gl
  renderer/gl/backend.cppm
  renderer/gl/framebuffer.cppm
  # renderer/gl/framebuffer_manager.cppm
  renderer/gl/gpu_buffer.cppm
  renderer/gl/index_buffer.cppm
  renderer/gl/shader.cppm
  renderer/gl/ssbo.cppm
  renderer/gl/state_cache.cppm
  renderer/gl/texture.cppm
  renderer/gl/uniform_buffer.cppm
  renderer/gl/vertex_buffer.cppm
  #   vk
  # renderer/vulkan/backend.cppm
)

target_sources(engine
  PRIVATE
  # Private implementation
  # normal .cpp files here

  # runtime
  runtime/runtime.cpp
  # platform
  platform/window.cpp
  # renderer
  # renderer/renderer.cpp
  # renderer/debug_drawer.cpp
  #   gl
  renderer/gl/backend.cpp
  # renderer/gl/framebuffer.cpp
  # renderer/gl/framebuffer_manager.cpp
  # renderer/gl/index_buffer.cpp
  # renderer/gl/ssbo.cpp
  # renderer/gl/uniform_buffer.cpp
  # renderer/gl/vertex_buffer.cpp
  #   vk
  # renderer/vulkan/backend.cpp
)

function(create_glad_lib)
  add_library(glad STATIC vendor/glad/src/gl.c)

  target_include_directories(glad
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include
  )
endfunction()
create_glad_lib()



# ==== Link Everything ====
target_link_libraries(engine
  PRIVATE
  $<$<BOOL:${ENABLE_TRACY}>:tracy_client>
  glm
  glfw
  glad
  freetype
  # RmlUi::Core
  # $<$<CONFIG:Debug>:RmlUi::Debugger>
  # imgui#?????????????????
)
if(WIN32)
  target_link_libraries(engine PRIVATE OpenGL::GL)
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
      target_link_libraries(engine PRIVATE Wayland::Client)
		else()
			message(STATUS "Wayland not found, falling back to X11")
			find_package(X11 REQUIRED)
      target_link_libraries(engine PRIVATE X11::X11)
		endif()
endif()


# ==== GLFW ====
function(configure_glfw)
  # Don't know about this
  # set(GLFW_USE_WAYLAND TRUE)
  set(GLFW_BUILD_WAYLAND ON)
  #[[WARNING THIS IS FOR DEBUG WITH RENDERDOC, IF NOT NEEDED REMOVE LINE]]
  set(GLFW_BUILD_X11 ON)
  set(GLFW_BUILD_SHARED_LIBS OFF)
  add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw
    build/glfw
  )
endfunction()

configure_glfw()

# ==== FreeType ====
function(configure_freetype)
  set(FT_DISABLE_ZLIB ON)
  set(FT_DISABLE_BZIP2 ON)
  set(FT_DISABLE_PNG ON)
  set(FT_DISABLE_HARFBUZZ)
  set(FT_DISABLE_BROTLI ON)
  add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/freetype
    build/freetype
  )
  add_library(Freetype::Freetype ALIAS freetype)
endfunction()
configure_freetype()

# ==== RmlUi ====
function(configure_rmlui)
  set(RMLUI_BUILD_SAMPLES OFF)
  set(RMLUI_BUILD_TESTS OFF)
  set(RMLUI_SHELL OFF)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RMLUI_BUILD_LIBRMLDEBUGGER ON)
  endif()
  set(RMLUI_FONT_ENGINE "freetype")
  add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/rmlui
    build/rmlui
  )
endfunction()
# configure_rmlui()

function(configure_glm)
  add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
    build/glm
  )
endfunction()
configure_glm()


add_custom_command(
    TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
)
