module;
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <cassert>
export module app_state;

import ui;
import framebuffer_manager;
import game_state;
import frame_context;
import window_context;
import input_manager;
// import chunk_manager;
import std;
import logger;
import gl_state;
import ecs;
import ecs_components;

export struct app_state {
  public:
    app_state(int width, int height, std::filesystem::path font_path)
      : ui(width, height, font_path),
        win_context(std::make_unique<WindowContext>(width, height, std::string(PROJECT_NAME) + std::string(" ") + std::string(PROJECT_VERSION))),
        fb_manager(*win_context.get()),
        g_state(width, height)
    {
      InputManager::get().setContext(win_context.get());
      InputManager::get().setMouseTrackingEnabled(true);
      glfwSetWindowUserPointer(win_context->window, this);
      glfwSetFramebufferSizeCallback(win_context->window,
          [](GLFWwindow* window, int width, int height) {
          auto* state = static_cast<app_state*>(glfwGetWindowUserPointer(window));
          if (!state) return;
          if (width <= 0 || height <= 0) return;
          state->on_framebuffer_resize(width, height);
          }
          );
    }
    app_state(const app_state& other) = delete;
    app_state& operator=(const app_state& other) = delete;
    app_state(app_state&& other) noexcept = delete;
    app_state& operator=(app_state&& other) noexcept = delete;
    ~app_state() {
      log::error("💀 app_state destroyed! ptr={}", (void*)this);
    }
    void on_framebuffer_resize(int width, int height) noexcept
    {
      win_context->set_framebuffer_size(width, height);

      GLState::set_viewport(0, 0, width, height);
      ImGui::SetNextWindowPos(ImVec2(width/* - 300*/, height), ImGuiCond_Always);
      assert(frame_ctx.active_camera.is_valid());
      g_state.ecs.get_component<Camera>(frame_ctx.active_camera)->aspect_ratio = float(static_cast<float>(win_context->get_framebuffer_width()) / win_context->get_framebuffer_height());

        g_state.ecs.for_each_component<RenderTarget>([&](const Entity e, RenderTarget& rt) {
          fb_manager.ensure(e, rt);
          log::info("rt.width = {}, rt.height = {}", rt.width, rt.height);
          ui.SetViewportSize(win_context->get_framebuffer_width(), win_context->get_framebuffer_height());
          }
          );
    }
    std::unique_ptr<WindowContext> win_context;
    FramebufferManager fb_manager;
    // InputManager input;
    // ChunkManager chunk_manager;
    UI ui;
    game_state g_state;
    frame_context frame_ctx;

};
