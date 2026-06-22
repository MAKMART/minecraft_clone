export module engine.core;

export import :types;
export import :logger;
export import :events;

import std;
export namespace engine::core {
  // Config
  struct engine_config {
    u8 fixed_hz = 60;
  };
  struct window_config {
    u32 width  = 1280;
    u32 height = 720;

    std::string title = "engine";

    bool fullscreen = false;
    bool resizable  = true;
    bool vsync = true;
  };

  struct renderer_config {
    u32 msaa   = 4;
  };
  struct app_config {
    engine_config engine;
    renderer_config renderer;
    window_config   window;
  };


  // Rendering
  struct render_command { };

	enum class extent_mode {
		fixed,
		follow_viewport
	};

	enum class framebuffer_attachment_type {
		color,
		depth,
		depth_stencil
	};
}
