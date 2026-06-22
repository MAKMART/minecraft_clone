export module engine.runtime;

import engine.core;
import engine.platform;
import engine.renderer;

export namespace engine {
  class App {
    public:
      virtual ~App() = default;
      // Disable copying to prevent pointer misalignment
      App(const App&) = delete;
      App& operator=(const App&) = delete;

      void stop() noexcept { running = false; }

      // "Hooks" for the game programmer to override
      virtual void on_init() noexcept = 0;
      virtual void on_update(f64 dt) noexcept = 0;
      virtual void on_render() noexcept = 0;
      virtual void on_shutdown() noexcept = 0;

      virtual engine::core::app_config get_config() const noexcept = 0;

      bool is_running() const noexcept { return running; }
    protected:
      App() = default;
      engine::platform::Window* window{nullptr};
      engine::platform::input_state* input{nullptr};
      engine::platform::input_action_map* input_map{nullptr};
      engine::renderer::Renderer* renderer{nullptr};

      // Allow engine::run to set the pointers up privately
      friend void run(App& application);
    private:
      bool running{false};
  };
  void run(App& application);
}
