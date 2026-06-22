import engine.runtime;
import engine.core;
import engine.platform;

import std;
class Application final : public engine::App {
  public:
    // Keep constructor empty or use it ONLY for standard, non-engine member variables
    Application() : App() {}
    void on_init() noexcept override {
      engine::core::logger::info("on_init override called");
      close_action = input_map->create_action("close_window");
      input_map->bind_key(close_action, engine::core::button::escape);
    }
    void on_update(f64 dt) noexcept override {
      // engine::core::logger::info("on_update override called");
      // engine::core::logger::info("dt: {}", dt);
      // engine::core::logger::info("fps: {}", static_cast<f64>(1.0) / dt);
      if (input_map->is_action_pressed(close_action, *input)) {
        window->request_close();
      }
      // engine::core::logger::info("FPS: {}", static_cast<f64>(1) / dt);
    }
    void on_render() noexcept override {
      // engine::core::logger::info("on_render override called");
    }
    void on_shutdown() noexcept override {
      engine::core::logger::info("on_shutdown override called");
    }
    engine::core::app_config get_config() const noexcept override {
        return {
          .renderer {  },
          .window { .width = 1920, .height = 1080, .title = "Window", .fullscreen = false, .vsync = true }
        };
    }
  private:
    engine::platform::action_id close_action{};
};

int main() {
  Application app;
  engine::run(app);
}
