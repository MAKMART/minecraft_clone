#pragma once
#include "core/defines.hpp"
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include "core/logger.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/TracyOpenGL.hpp>
#endif
// class Renderer;
//	Maybe implement in future

struct WindowContext {
	WindowContext(int _width, int _height, std::string _title) noexcept;
	WindowContext(const WindowContext&) = delete;
	WindowContext& operator=(const WindowContext&) = delete;
	WindowContext(WindowContext&& other) noexcept;
	WindowContext& operator=(WindowContext&& other) noexcept;
	~WindowContext() noexcept;
	void toggle_fullscreen() noexcept;

	constexpr int getWidth() const noexcept { return width; }
	constexpr int getHeight() const noexcept { return height; }
	constexpr bool isFullscreen() const noexcept { return is_fullscreen; }

	GLFWwindow*  window = nullptr;
	// Renderer* renderer = nullptr;

      private:
	int         width, height;
	std::string title;
	float       aspect_ratio;
	b8          is_fullscreen = false;
	i32         windowedWidth, windowedHeight;
	i32         windowedPosX, windowedPosY;

	void create_window();
	static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
};
