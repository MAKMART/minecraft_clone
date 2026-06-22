module;
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
export module engine.platform;

import engine.core;

export import :window;
export import :input_state;
export import :input_action_map;

export namespace engine::platform {
  struct platform_context final {
    platform_context() {
      if (!glfwInit()) {
        core::logger::system_error("GLFW", "Failed to initialize for windowing");
        std::exit(1);
      }
#if defined(DEBUG)
      glfwSetErrorCallback([](i32 code, const char* desc)
          {
          core::logger::system_error("GLFW", "{} : {}", code, desc);
          });
#endif
    }
    [[nodiscard("Use the time you queried")]] static f64 get_time() noexcept { return glfwGetTime(); }

    ~platform_context() noexcept {
      glfwTerminate();
    }

    platform_context(const platform_context&) = delete;
    platform_context& operator=(const platform_context&) = delete;
  };
}
