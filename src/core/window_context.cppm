module;
#include <gl.h>
#include <GLFW/glfw3.h>
export module window_context;
// class Renderer;
//	Maybe implement in future
import std;

export struct WindowContext {
	WindowContext(int _width, int _height, std::string _title) noexcept;
	WindowContext(const WindowContext&) = delete;
	WindowContext& operator=(const WindowContext&) = delete;
	WindowContext(WindowContext&& other) noexcept { *this = std::move(other); }
	WindowContext& operator=(WindowContext&& other) noexcept;
	~WindowContext() noexcept;
	void toggle_fullscreen() noexcept;

	constexpr int get_width() const noexcept { return width; }
	constexpr int get_height() const noexcept { return height; }
	constexpr bool is_fullscreen() const noexcept { return fullscreen; }
  bool should_close() const noexcept { return glfwWindowShouldClose(window); }

	GLFWwindow*  window = nullptr;
	// Renderer* renderer = nullptr;

      private:
	int         width, height;
	std::string title;
	float       aspect_ratio;
	bool        fullscreen = false;
	bool		v_sync = false;
	int         windowedWidth, windowedHeight;
	int         windowedPosX, windowedPosY;

	void create_window();
	static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
};
