module;
#include <cassert>
#include <imgui.h>
export module app;
/*
 *
 * TODO: IMPLEMENT THE APPSTATE AND THE MAIN MENU WITH THE NEW UI SYSTEM
 */
import core;
import shader;
import chunk_manager;
import texture;
import std;
import input_manager;
import ui;
import framebuffer_manager;
import game_state;
import frame_context;
import window_context;

export struct App {
  public:
    explicit App(int width = 1920, int height = 1080);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    App(App&&) noexcept = delete;
    App& operator=(App&&) noexcept = delete;

    void run() noexcept;

  private:
    void begin_frame() noexcept;
    void update() noexcept;
    void render() noexcept;
    void end_frame() noexcept;

    std::unique_ptr<WindowContext> win_context;
    FramebufferManager fb_manager;
    UI ui;
    game_state g_state;
    frame_context frame_ctx;

    void on_framebuffer_resize(int width, int height) noexcept;
    void on_scroll_callback(double xoffset, double yoffset) noexcept {
      input.scroll_callback(xoffset, yoffset);
    };
    void on_cursor_pos_callback(double xpos, double ypos) noexcept {
      input.cursor_pos_callback(xpos, ypos);
    };
    void on_mouse_button_callback(int button, int action, int mods) noexcept {
      input.mouse_button_callback(button, action, mods);
    };
    void on_key_callback(int key, int scancode, int action, int mods) noexcept {
      input.key_callback(key, scancode, action, mods);
    };
    void on_window_focus_callback(bool focused) noexcept {
      input.window_focus_callback(focused);
    };

    Shader playerShader;
    Shader fb_debug;
    Shader fb_player;
    ChunkManager manager;
    Texture Atlas;
    static constexpr std::string_view cube_faces[6] = {
      "px.png",// +x
      "nx.png",// -x
      "ny.png",// -y
      "py.png",// +y
      "pz.png",// +z
      "nz.png" // -z
    };
    // --- CROSSHAIR STUFF ---
    Shader crossHairshader;
    Texture crossHairTexture;
    float crosshair_size = 0.3f;
    static constexpr int CrosshairIndices[6] = {
      0, 1, 2,
      2, 3, 0
    };

    double last_frame_time = 0.0;
    InputManager input;
    GLuint cube_id;
    GLuint crosshairVAO;
    GLuint VAO;
};
