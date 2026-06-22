module;
#include <GLFW/glfw3.h>
#include <cassert>
export module engine.platform:window;
import std;
import engine.core;

export namespace engine::platform {
  struct Window final {
    public:
    Window(const engine::core::window_config& config, engine::core::events::event_dispatcher& dispatcher) noexcept;
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept = default;
    Window& operator=(Window&&) noexcept = delete;
    ~Window() noexcept;
    void toggle_fullscreen() noexcept;

    i32 get_window_width() const noexcept { return window_width; }
    i32 get_window_height() const noexcept { return window_height; }
    i32 get_framebuffer_width() const noexcept { return framebuffer_width; }
    i32 get_framebuffer_height() const noexcept { return framebuffer_height; }

    f32 get_window_aspect_ratio() const noexcept { return window_width / (f32)window_height; }
    f32 get_framebuffer_aspect_ratio() const noexcept { return framebuffer_width / (f32)framebuffer_height; }
    bool is_fullscreen() const noexcept { return fullscreen; }
    bool should_close() const noexcept { assert(window_handle); return glfwWindowShouldClose(window_handle); }
    void request_close() noexcept { assert(window_handle); glfwSetWindowShouldClose(window_handle, GLFW_TRUE); }

    inline void present() const noexcept { assert(window_handle); glfwSwapBuffers(window_handle); }
    inline void poll_events() const noexcept { glfwPollEvents(); }
    // bool is_valid() const noexcept { return window_handle != nullptr; }
    // explicit operator bool() const noexcept { return is_valid(); }
    void register_callbacks() noexcept;

    void set_input_mode(engine::core::cursor_mode mode) {
      using namespace engine::core;
      switch (mode) {
        case cursor_mode::visible:
          glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
          break;
        case cursor_mode::hidden:
          glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
          break;
        case cursor_mode::locked:
          glfwSetInputMode(window_handle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          break;
        default:
          break;
      }
    }

    void set_scroll_callback(std::function<void(f64, f64)> callback) {
      scroll_callback = std::move(callback);
    }
    void set_cursor_position_callback(std::function<void(f64, f64)> callback) {
      cursor_position_callback = std::move(callback);
    }
    private:
    engine::core::events::event_dispatcher& dispatcher;
    i32 window_width{};
    i32 window_height{};
    std::string title;
    bool fullscreen{false};
    bool v_sync{false};
    bool resizable{true};

    i32 framebuffer_width{};
    i32 framebuffer_height{};
    f32 dpi_scale_x{1.0f};
    f32 dpi_scale_y{1.0f};

    i32 windowedWidth{};
    i32 windowedHeight{};
    i32 windowedPosX{};
    i32 windowedPosY{};

    std::expected<void, std::string> create_window();
    void destroy_window() noexcept {
      if (window_handle) {
        glfwMakeContextCurrent(nullptr);
        glfwDestroyWindow(window_handle);
        window_handle = nullptr;
      }
    }
    GLFWwindow* window_handle = nullptr;
    std::function<void(f64, f64)> scroll_callback;
    std::function<void(f64, f64)> cursor_position_callback;
  };
}
