#pragma once
#include "defines.h"
#include "InputManager.hpp"
#include "Player/Player.h"
#include "Shader.h"
#include "UI.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include "Timer.h"
#include "logger.hpp"

class Application {
  public:
    Application(int width, int height);
    ~Application(void);
    void Run(void);

  private:
    GLFWwindow *window;
    std::string title = std::string(PROJECT_NAME) + std::string(" ") + std::string(PROJECT_VERSION);
    u16 width, height;
    float aspectRatio;
    glm::vec4 backgroundColor;

    u8 nbFrames = 0;
    f32 deltaTime = 0.0f;
    f32 lastFrame = 0.0f;

    b8 isFullscreen = false;
    b8 renderUI = true;
    b8 renderTerrain = true;
#if defined(DEBUG)
    b8 debugRender = false;
#endif
    i32 windowedWidth, windowedHeight;
    i32 windowedPosX, windowedPosY;

    b8 firstMouse = true;

    std::unique_ptr<Shader> playerShader;
    std::unique_ptr<ChunkManager> chunkManager;
    std::unique_ptr<Player> player;
    std::shared_ptr<InputManager> input;
    std::unique_ptr<UI> ui;

    void initWindow(void);
    void handleFullscreenToggle(GLFWwindow *window);
    static void framebuffer_size_callback(GLFWwindow *window, int width,
                                          int height);
    static void cursor_callback(GLFWwindow *window, double xpos, double ypos);
    static void mouse_callback(GLFWwindow *window, int button, int action,
                               int mods);
    static void scroll_callback(GLFWwindow *window, double xoffset,
                                double yoffset);
    static void key_callback(GLFWwindow *window, int key, int scancode,
                             int action, int mods);
    void processInput();
    float getFPS();

    // -- Crosshair ---
    std::unique_ptr<Shader> crossHairshader;
    std::unique_ptr<Texture> crossHairTexture;
    // Use std::vector for dynamic size safety
    std::vector<float> Crosshairvertices;
    static constexpr unsigned int CrosshairIndices[6] = {
        0, 1, 2, // First triangle
        2, 3, 0  // Second triangle
    };
    float crosshairSize;

    unsigned int crosshairVAO, crosshairVBO, crosshairEBO;

    
    static void MessageCallback(GLenum source, GLenum type, GLuint id,
                                GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam);
};
