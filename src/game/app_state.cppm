export module app_state;

import ui;
import framebuffer_manager;
import game_state;
import frame_context;
import window_context;
// import input_manager;
// import chunk_manager;
import std;
import logger;

export struct app_state {
  public:
    app_state(int width, int height, std::filesystem::path font_path)
      : ui(width, height, font_path),
        win_context(std::make_unique<WindowContext>(width, height, std::string(PROJECT_NAME) + std::string(" ") + std::string(PROJECT_VERSION))),
        fb_manager(*win_context.get()),
        g_state(width, height)
    {
      // InputManager::get().setContext(win_context.get());
      // InputManager::get().setMouseTrackingEnabled(true);
    }
    app_state(const app_state& other) = delete;
    app_state& operator=(const app_state& other) = delete;
    app_state(app_state&& other) noexcept = delete;
    app_state& operator=(app_state&& other) noexcept = delete;
    ~app_state() {
      log::error("💀 app_state destroyed! ptr={}", (void*)this);
    }
    std::unique_ptr<WindowContext> win_context;
    FramebufferManager fb_manager;
    // InputManager input;
    // ChunkManager chunk_manager;
    UI ui;
    game_state g_state;
    frame_context frame_ctx;

};
