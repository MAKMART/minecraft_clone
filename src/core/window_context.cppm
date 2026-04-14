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
  void set_framebuffer_size(int w, int h) noexcept {
    framebuffer_width = w;
    framebuffer_height = h;
  }

  void set_window_size(int w, int h) noexcept {
    window_width = w;
    window_height = h;
  }


	inline int get_window_width() const noexcept { return window_width; }
	inline int get_window_height() const noexcept { return window_height; }
	inline int get_framebuffer_width() const noexcept { return framebuffer_width; }
	inline int get_framebuffer_height() const noexcept { return framebuffer_height; }

  inline float get_window_aspect_ratio() const noexcept { return window_width / (float)window_height; }
  inline float get_framebuffer_aspect_ratio() const noexcept { return framebuffer_width / (float)framebuffer_height; }
	constexpr bool is_fullscreen() const noexcept { return fullscreen; }
  bool should_close() const noexcept { return glfwWindowShouldClose(window); }

	GLFWwindow*  window = nullptr;
	// Renderer* renderer = nullptr;

  private:
  int window_width = -1, window_height = -1;
  int framebuffer_width = -1, framebuffer_height = -1;
  float dpi_scale_x = 1.0f, dpi_scale_y = 1.0f;

	std::string title;
	bool        fullscreen = false;
  bool		    v_sync = false;
	int         windowedWidth, windowedHeight;
	int         windowedPosX, windowedPosY;

	void create_window();
	static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
};
