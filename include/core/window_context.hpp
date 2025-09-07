#pragma once
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_image.h>
#include "core/defines.hpp"
#include "core/logger.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/TracyOpenGL.hpp>
#endif
class Application;
// class Renderer;
//	Maybe implement in future

struct WindowContext {
	WindowContext(int _width, int _height, std::string _title, Application* _app) : width(_width), height(_height), title(_title), app(_app)
	{
		create_window();
		glfwSetWindowUserPointer(window, this);
	}
	~WindowContext()
	{
		if (window) {
			glfwDestroyWindow(window);
		}
	}
	void toggle_fullscreen()
	{
		if (is_fullscreen) {
			glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
			is_fullscreen = false;
		} else {
			const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
			glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
			glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
			glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
			is_fullscreen = true;
		}
	}

	int getWidth() const
	{
		return width;
	}
	int getHeight() const
	{
		return height;
	}
	bool isFullscreen() const
	{
		return is_fullscreen;
	}
	Application* app    = nullptr;
	GLFWwindow*  window = nullptr;
	// Renderer* renderer = nullptr;

      private:
	int         width, height;
	std::string title;
	float       aspect_ratio;
	b8          is_fullscreen = false;
	i32         windowedWidth, windowedHeight;
	i32         windowedPosX, windowedPosY;

	void create_window()
	{
		if (!glfwInit()) {
			log::error("Failed to initialize GLFW for windowing");
			std::exit(1);
		}

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
#endif

#if defined(DEBUG)
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE); // !!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!
		glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
		glfwWindowHint(GLFW_SAMPLES, MSAA_STRENGHT);

		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_DEPTH_BITS, DEPTH_BITS);
		glfwWindowHint(GLFW_STENCIL_BITS, STENCIL_BITS);
		glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
		glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

		// Get primary monitor and its video mode
		GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode    = glfwGetVideoMode(monitor);

		window = is_fullscreen ? glfwCreateWindow(mode->width, mode->height, title.c_str(), monitor, nullptr)
		                       : glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
		if (!window) {
			glfwTerminate();
			log::error("Failed to create GLFW window");
			std::exit(1);
		}
		glfwMakeContextCurrent(window);

		if (glfwRawMouseMotionSupported()) {
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		} else {
			log::info("Raw Mouse Motion not supported!");
		}

		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			log::error("Failed to initialize GLEW!");
			std::exit(1);
		}

		// CONTEX CREATION TERMINATED

#if defined(TRACY_ENABLE)
		TracyGpuContext;
#endif

		// --- DEBUG SETUP ---
#if defined(DEBUG)
		if (glfwExtensionSupported("GL_KHR_debug")) {
			log::info("Debug Output is supported!");
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(WindowContext::MessageCallback, 0);
			GLenum err2 = glGetError();
			if (err2 != GL_NO_ERROR) {
				log::error("Failed to set debug callback");
			}
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
		} else {
			log::info("Debug Output is not supported by this OpenGL context!");
		}
#endif

		// Set window icon
		int            icon_width, icon_height, icon_channels;
		unsigned char* icon_data = stbi_load(WINDOW_ICON_DIRECTORY.string().c_str(), &icon_width, &icon_height, &icon_channels, 4);

		if (icon_data) {
			log::info("Loading window icon {} x {} with {} channels", icon_width, icon_height, icon_channels);

			GLFWimage image;
			image.width  = icon_width;
			image.height = icon_height;
			image.pixels = icon_data;

			glfwSetWindowIcon(window, 1, &image);
			stbi_image_free(icon_data);
		} else {
			log::error("Failed to load window icon: {}", std::string(stbi_failure_reason()));
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
			log::error("Failed to set window icon, OpenGL error: {}", errorMessage);
		}

		glViewport(0, 0, width, height);
		glfwSwapInterval(V_SYNC ? 1 : 0);

		// OpenGL state setup
		if (FACE_CULLING) {
			glFrontFace(GL_CCW);
			glCullFace(GL_BACK);
			glEnable(GL_CULL_FACE);
		}
		if (DEPTH_TEST) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(DEPTH_FUNC);
		}
		if (BLENDING) {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		if (MSAA)
			glEnable(GL_MULTISAMPLE);

		aspect_ratio = (float)width / (float)height;
	}
	static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	{
		(void)length;
		(void)userParam;
		// Map severity to log level & severity text
		log::LogLevel level;
		std::string   severityText;

		switch (severity) {
		case GL_DEBUG_SEVERITY_HIGH:
			level        = log::LogLevel::ERROR;
			severityText = "High";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			level        = log::LogLevel::WARNING;
			severityText = "Medium";
			break;
		case GL_DEBUG_SEVERITY_LOW:
			return;
			level        = log::LogLevel::INFO;
			severityText = "Low";
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			return;
			level        = log::LogLevel::INFO;
			severityText = "Notification";
			break;
		default:
			level        = log::LogLevel::INFO;
			severityText = "Unknown";
			break;
		}

		// Map source enum to string
		const char* sourceStr = nullptr;
		switch (source) {
		case GL_DEBUG_SOURCE_API:
			sourceStr = "API";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			sourceStr = "Window System";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			sourceStr = "Shader Compiler";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			sourceStr = "Third Party";
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			sourceStr = "Application";
			break;
		case GL_DEBUG_SOURCE_OTHER:
			sourceStr = "Other";
			break;
		default:
			sourceStr = "Unknown";
			break;
		}

		// Map type enum to string
		const char* typeStr = nullptr;
		switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			typeStr = "Error";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			typeStr = "Deprecated Behavior";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			typeStr = "Undefined Behavior";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			typeStr = "Performance";
			break;
		case GL_DEBUG_TYPE_MARKER:
			typeStr = "Marker";
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			typeStr = "Push Group";
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			typeStr = "Pop Group";
			break;
		default:
			typeStr = "Unknown";
			break;
		}

		// Construct a detailed log message with structured info
		log::system_structured(
		    "OpenGL",
		    level,
		    message,
		    {{"\nSource", sourceStr},
		     {"\nType", typeStr},
		     {"\nSeverity", severityText},
		     {"\nMessageID", std::to_string(id) + '\n'}});
	}
};
