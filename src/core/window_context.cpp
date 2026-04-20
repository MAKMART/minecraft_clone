module;
#define GLAD_GL_IMPLEMENTATION
#include <gl.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#if defined(TRACY_ENABLE)
#include <tracy/TracyOpenGL.hpp>
#endif
module window_context;
// class Renderer;
//	Maybe implement in future
import core;
import std;
import logger;
import gl_state;
import debug;

WindowContext::WindowContext(int _width, int _height, std::string _title) noexcept : window_width(_width), window_height(_height), title(_title) 
{
	create_window();
}
WindowContext& WindowContext::operator=(WindowContext&& other) noexcept
{
	if (this != &other) {
		if (window)
			glfwDestroyWindow(window);

		window = other.window;
		other.window = nullptr;

		window_width = other.window_width;
		window_height = other.window_height;
		title = std::move(other.title);
		fullscreen = other.fullscreen;
		windowedWidth = other.windowedWidth;
		windowedHeight = other.windowedHeight;
		windowedPosX = other.windowedPosX;
		windowedPosY = other.windowedPosY;
	}
	return *this;
}
WindowContext::~WindowContext() noexcept
{
  log::error("🔥 WindowContext destroyed! ptr={}", (void*)this);
	if (window)
		glfwDestroyWindow(window);
	glfwTerminate();
}
void WindowContext::toggle_fullscreen() noexcept
{
	if (fullscreen) {
		glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
		fullscreen = false;
	} else {
		const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
		glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
		glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
		fullscreen = true;
	}
	glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
	// GLState::set_viewport(0, 0, width, height);
}

void WindowContext::create_window()
{
#if defined(DEBUG)
  glfwSetErrorCallback([](int code, const char* desc)
      {
      log::system_error("GLFW", "{} : {}", code, desc);
      });
#endif
	if (!glfwInit()) {
		log::system_error("GLFW", "Failed to initialize for windowing");
		std::exit(1);
	}

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
    log::system_info("GLFW", "running on X11");
		// XWayland-safe
		glfwWindowHint(GLFW_SAMPLES, 0);
		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
	} else if (platform == GLFW_PLATFORM_WAYLAND) {
    log::system_info("GLFW", "running on Wayland");
  } else if (platform == GLFW_PLATFORM_WIN32) {
    log::system_info("GLFW", "running on WIN32");
  }

	//glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE); // !!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	//glfwWindowHint(GLFW_SAMPLES, MSAA_STRENGHT);

	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	//glfwWindowHint(GLFW_DEPTH_BITS, DEPTH_BITS);
	//glfwWindowHint(GLFW_STENCIL_BITS, STENCIL_BITS);
	if (platform != GLFW_PLATFORM_X11) {
		glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
	}
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

	// Get primary monitor and its video mode
	GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

	window = fullscreen ? glfwCreateWindow(mode->width, mode->height, title.c_str(), monitor, nullptr)
		: glfwCreateWindow(window_width, window_height, title.c_str(), nullptr, nullptr);
	if (!window) {
		glfwTerminate();
		log::system_error("GLFW", "Failed to create window");
		std::exit(1);
	}
	glfwMakeContextCurrent(window);
	if (glfwRawMouseMotionSupported()) {
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	} else {
		log::system_info("GLFW", "Raw Mouse Motion not supported!");
	}

  int version = gladLoadGL(glfwGetProcAddress);
	if (!version) {
		log::error("Failed to initialize GLAD!");
		std::exit(1);
	} else {
    log::info("Initialized GLAD {}.{}", GLAD_VERSION_MAJOR(version),  GLAD_VERSION_MINOR(version));
  }
  // log::system_info("GLFW", "glGetString(GL_VERSION) == {}", (char*)glGetString(GL_VERSION));
  // log::system_info("GLFW", "glGetString(GL_VENDOR) == {}", (char*)glGetString(GL_VENDOR));
  // log::system_info("GLFW", "glGetString(GL_RENDERER) == {}", (char*)glGetString(GL_RENDERER));

  std::cout << "glGetString(GL_VERSION)" << glGetString(GL_VERSION) << std::endl;
  std::cout << "glGetString(GL_VENDOR)" << glGetString(GL_VENDOR) << std::endl;
  std::cout << "glGetString(GL_RENDERER)" << glGetString(GL_RENDERER) << std::endl;

  glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);


	// CONTEX CREATION TERMINATED

#if defined(TRACY_ENABLE)
	TracyGpuContext;
#endif

	// --- DEBUG SETUP ---
#if defined(DEBUG)
	if (glfwExtensionSupported("GL_KHR_debug")) {
		log::system_info("GLFW", "Debug Output is supported!");
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(WindowContext::MessageCallback, 0);
		GLenum err2 = glGetError();
		if (err2 != GL_NO_ERROR) {
			log::error("Failed to set debug callback");
		}
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	} else {
		log::system_info("GLFW", "Debug Output is not supported by this OpenGL context!");
	}
#endif

	// Set window icon
	int            icon_width, icon_height, icon_channels;
	unsigned char* icon_data = stbi_load(WINDOW_ICON_DIRECTORY.string().c_str(), &icon_width, &icon_height, &icon_channels, 4);

	if (icon_data) {
		// log::info("Loading window icon {} x {} with {} channels", icon_width, icon_height, icon_channels);

		GLFWimage image;
		image.width  = icon_width;
		image.height = icon_height;
		image.pixels = icon_data;

		glfwSetWindowIcon(window, 1, &image);
		stbi_image_free(icon_data);
	} else {
		log::system_error("WINDOW", "Failed to load window icon: {}", std::string(stbi_failure_reason()));
	}

	GLenum err3 = glGetError();
	if (err3 != GL_NO_ERROR) {
		std::string errorMessage;
		switch (err3) {
			case GL_INVALID_VALUE:
				errorMessage = "Invalid value";
				break;
			case GL_INVALID_OPERATION:
				errorMessage = "Invalid operation";
				break;
			default:
				errorMessage = "Unknown OpenGL error (" + std::to_string(err3) + ")";
		}
		log::system_error("WINDOW", "Failed to set window icon, OpenGL error: {}", errorMessage);
	}

	glfwSwapInterval(v_sync ? 1 : 0);


	GLState::init_capabilities();
	GLState::sync();
}
void WindowContext::MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	(void)length;
	(void)userParam;
	// Map severity to log level & severity text
	log::level level;

	switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			level        = log::level::ERROR;
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			level        = log::level::WARNING;
			break;
		case GL_DEBUG_SEVERITY_LOW:
			return;
			level        = log::level::INFO;
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			return;
			level        = log::level::INFO;
			break;
		default:
			level        = log::level::INFO;
			break;
	}

  std::string_view severityText = gl_enum_to_string(severity);
  std::string_view sourceStr = gl_enum_to_string(source);
  std::string_view typeStr = gl_enum_to_string(type);

	log::structured(
			level,
			message,
			{
				{"\nType", typeStr},
				{"\nSource", sourceStr},
				//{"\nSeverity", severityText},
				{"\nMessageID", std::to_string(id) + '\n'}
			}
			);
}
