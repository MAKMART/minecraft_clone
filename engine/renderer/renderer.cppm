export module engine.renderer;
import std;
import engine.core;

// export import :debug_drawer;
import :gl_backend;

using namespace engine::core;
export namespace engine::renderer {

  struct viewport_extent {
    u32 width{};
    u32 height{};
  };

  enum class RENDER_API : u8 {
    OPENGL,
    VULKAN,
  };

  class Renderer {
    public:
      Renderer() {
        auto result = gl_backend::init();
        // if (api == RENDER_API::VULKAN)
        //   result = vk_backend::init();
        if (!result) {
          engine::core::logger::system_error("Renderer", "{}", result.error());
          std::terminate();
        }
      }
      ~Renderer() = default;

      void shutdown() {
        switch (api) {
          case RENDER_API::OPENGL:
            gl_backend::shutdown();
            break;
          case RENDER_API::VULKAN:
            // vk_backend::shutdown();
            break;
        }

      };
      void submit(const render_command& command);
      void render();

      void start_frame() {};
      void end_frame() {};

    private:
      RENDER_API api{RENDER_API::OPENGL};
      std::vector<render_command> commands;
  };
}
