module;
#include <GLFW/glfw3.h>
#include <stb_image.h>
#if defined(TRACY_ENABLE)
#include <tracy/TracyOpenGL.hpp>
#endif
module engine.platform;
import engine.core;

using namespace engine::core;

namespace  engine::platform {
  Window::Window(const engine::core::window_config& config, engine::core::events::event_dispatcher& dispatcher) noexcept
    : dispatcher(dispatcher), window_width(config.width), window_height(config.height),
    title(config.title), fullscreen(config.fullscreen),
    v_sync(config.vsync),
    resizable(config.resizable) {
    if (auto result = create_window(); !result) {
      engine::core::logger::system_error("Window", "{}", result.error());
      std::terminate();
    }
  }
  Window::~Window() noexcept {
    destroy_window();
  }
  void Window::toggle_fullscreen() noexcept {
    if (fullscreen) {
      glfwSetWindowMonitor(window_handle, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
    } else {
      const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
      glfwGetWindowPos(window_handle, &windowedPosX, &windowedPosY);
      glfwGetWindowSize(window_handle, &windowedWidth, &windowedHeight);
      glfwSetWindowMonitor(window_handle, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
    }
    fullscreen = !fullscreen;
    glfwGetFramebufferSize(window_handle, &framebuffer_width, &framebuffer_height);
    // GLState::set_viewport(0, 0, width, height);
  }

  std::expected<void, std::string> Window::create_window() {
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Warning this behaves differently on MacOS
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
#if defined(DEBUG)
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    const int platform = glfwGetPlatform();
    // Platform-specific
    if (platform == GLFW_PLATFORM_X11) {
      engine::core::logger::system_info("GLFW\t", "running on X11");
      // XWayland-safe
      glfwWindowHint(GLFW_SAMPLES, 0);
      glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
    } else if (platform == GLFW_PLATFORM_WAYLAND) {
      engine::core::logger::system_info("GLFW\t", "running on Wayland");
    } else if (platform == GLFW_PLATFORM_WIN32) {
      engine::core::logger::system_info("GLFW\t", "running on WIN32");
    }

    //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE); // !!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!
    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    //glfwWindowHint(GLFW_SAMPLES, MSAA_STRENGHT);

    if (resizable) {
      glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    } else {
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }
    //glfwWindowHint(GLFW_DEPTH_BITS, DEPTH_BITS);
    //glfwWindowHint(GLFW_STENCIL_BITS, STENCIL_BITS);
    if (platform != GLFW_PLATFORM_X11) {
      glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    if(fullscreen) {
      // Get primary monitor and its video mode
      GLFWmonitor* monitor = glfwGetPrimaryMonitor();
      if(!monitor) {
        const char* desc = nullptr;
        glfwGetError(&desc);
        return std::unexpected(desc ? std::string(desc) : "Couldn't get the primary monitor");
      }

      const GLFWvidmode* mode = glfwGetVideoMode(monitor);
      if(!mode) {
        const char* desc = nullptr;
        glfwGetError(&desc);
        return std::unexpected(desc ? std::string(desc) : "Couldn't get the video mode on the monitor");
      }
      window_handle = glfwCreateWindow(mode->width, mode->height, title.c_str(), monitor, nullptr);
    } else {
      window_handle = glfwCreateWindow(window_width, window_height, title.c_str(), nullptr, nullptr);
    }

    if (!window_handle) {
      const char* desc = nullptr;
      glfwGetError(&desc);
      return std::unexpected(desc ? std::string(desc) : "failed to create window");
    }


    glfwSetWindowUserPointer(window_handle, this);
    glfwMakeContextCurrent(window_handle);

    if (glfwRawMouseMotionSupported()) {
      glfwSetInputMode(window_handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
      engine::core::logger::system_info("GLFW", "Raw Mouse Motion not supported!");
    }

    glfwGetFramebufferSize(window_handle, &framebuffer_width, &framebuffer_height);

    // CONTEX CREATION TERMINATED

#if defined(TRACY_ENABLE)
    TracyGpuContext;
#endif

    // --- DEBUG SETUP ---
    // #if defined(DEBUG)
    // 	if (glfwExtensionSupported("GL_KHR_debug")) {
    // 		logger::system_info("GLFW", "Debug Output is supported!");
    // 		glEnable(GL_DEBUG_OUTPUT);
    // 		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // 		glDebugMessageCallback(WindowContext::MessageCallback, 0);
    // 		GLenum err2 = glGetError();
    // 		if (err2 != GL_NO_ERROR) {
    // 			logger::error("Failed to set debug callback");
    // 		}
    // 		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    // 	} else {
    // 		logger::system_info("GLFW", "Debug Output is not supported by this OpenGL context!");
    // 	}
    // #endif

    // Set window icon
    // {
    //   i32 icon_width{};
    //   i32 icon_height{};
    //   i32 icon_channels{};
    //   u8* icon_data = stbi_load(WINDOW_ICON_DIRECTORY.string().c_str(), &icon_width, &icon_height, &icon_channels, 4);
    //
    //   if (icon_data) {
    //     // logger::info("Loading window icon {} x {} with {} channels", icon_width, icon_height, icon_channels);
    //
    //     GLFWimage image;
    //     image.width  = icon_width;
    //     image.height = icon_height;
    //     image.pixels = icon_data;
    //
    //     glfwSetWindowIcon(window_handle, 1, &image);
    //     stbi_image_free(icon_data);
    //   } else {
    //     logger::system_error("WINDOW", "Failed to load window icon: {}", std::string(stbi_failure_reason()));
    //   }
    //
    // }
    glfwSwapInterval((i32)v_sync);

    // GLenum err3 = glGetError();
    // if (err3 != GL_NO_ERROR) {
    // 	std::string errorMessage;
    // 	switch (err3) {
    // 		case GL_INVALID_VALUE:
    // 			errorMessage = "Invalid value";
    // 			break;
    // 		case GL_INVALID_OPERATION:
    // 			errorMessage = "Invalid operation";
    // 			break;
    // 		default:
    // 			errorMessage = "Unknown OpenGL error (" + std::to_string(err3) + ")";
    // 	}
    // 	logger::system_error("WINDOW", "Failed to set window icon, OpenGL error: {}", errorMessage);
    // }



    // GLState::init_capabilities();
    // GLState::sync();
    return {};
  }
  void Window::register_callbacks() noexcept {
    // All callbacks must be set to a valid function pointer
    // assert(scroll_callback || cursor_position_callback);
    glfwSetWindowSizeCallback(window_handle,
        [](GLFWwindow* window, i32 width, i32 height)
        {
        auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->dispatcher.dispatch(engine::core::events::window_resize{ .width = static_cast<u32>(width), .height = static_cast<u32>(height)});
        });
    glfwSetFramebufferSizeCallback(window_handle,
        [](GLFWwindow* window, i32 width, i32 height)
        {
        // Dispatch a framebuffer_size_callaback event
        // auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        // ptr->dispatcher.dispatch(engine::core::events::window_resize{ .width = static_cast<u32>(width), .height = static_cast<u32>(height)});
        });
    glfwSetKeyCallback(window_handle,
        [](GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods)
        {
        auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->dispatcher.dispatch(engine::core::events::key_event{ .key = key, .scancode = scancode, .action = action, .mods = mods});
        });
    glfwSetScrollCallback(window_handle,
        [](GLFWwindow* window, f64 x_offset, f64 y_offset)
        {
        auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (ptr->scroll_callback)
        ptr->scroll_callback(x_offset, y_offset);
        });
    glfwSetCursorPosCallback(window_handle,
        [](GLFWwindow* window, f64 x_pos, f64 y_pos)
        {
        auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (ptr->cursor_position_callback)
        ptr->cursor_position_callback(x_pos, y_pos);
        });
    glfwSetMouseButtonCallback(window_handle,
        [](GLFWwindow* window, i32 button, i32 action, i32 mods)
        {
        auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->dispatcher.dispatch(engine::core::events::mouse_button{ .button = button, .action = action, .mods = mods });
        });
    glfwSetWindowFocusCallback(window_handle,
        [](GLFWwindow* window, i32 focused)
        {
        auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->dispatcher.dispatch(engine::core::events::window_focus{static_cast<bool>(focused)});
        });
    glfwSetWindowCloseCallback(window_handle,
        [](GLFWwindow* window)
        {
        auto* ptr = static_cast<Window*>(glfwGetWindowUserPointer(window));
        ptr->dispatcher.dispatch(engine::core::events::window_close{});
        });
  }

}

// static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
// {
// 	(void)length;
// 	(void)userParam;
// 	// Map severity to log level & severity text
// 	logger::level level;
//
// 	switch (severity) {
// 		case GL_DEBUG_SEVERITY_HIGH:
// 			level        = logger::level::ERROR;
// 			break;
// 		case GL_DEBUG_SEVERITY_MEDIUM:
// 			level        = logger::level::WARNING;
// 			break;
// 		case GL_DEBUG_SEVERITY_LOW:
// 			return;
// 			level        = logger::level::INFO;
// 			break;
// 		case GL_DEBUG_SEVERITY_NOTIFICATION:
// 			return;
// 			level        = logger::level::INFO;
// 			break;
// 		default:
// 			level        = logger::level::INFO;
// 			break;
// 	}
//
//   std::string_view severityText = gl_enum_to_string(severity);
//   std::string_view sourceStr = gl_enum_to_string(source);
//   std::string_view typeStr = gl_enum_to_string(type);
//
// 	logger::structured(
// 			level,
// 			message,
// 			{
// 				{"\nType", typeStr},
// 				{"\nSource", sourceStr},
// 				//{"\nSeverity", severityText},
// 				{"\nMessageID", std::to_string(id) + '\n'}
// 			}
// 			);
// }
