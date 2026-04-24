module;
#include <gl.h>
export module app;
/*
 *
 * TODO: IMPLEMENT THE APPSTATE AND THE MAIN MENU WITH THE NEW UI SYSTEM
 */

import core;
import app_state;
import shader;
import chunk_manager;
import texture;
import std;
import input_manager;


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
    app_state state;
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

    double lastFrame = 0.0;
    InputManager& input = InputManager::get();
    GLuint cube_id;
    GLuint crosshairVAO;
    GLuint VAO;
};
