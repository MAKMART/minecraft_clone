#include "core/defines.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <sstream>

class OpenGLError : public std::runtime_error {
  public:
    explicit OpenGLError(GLenum errorCode)
        : std::runtime_error(buildMessage(errorCode)), errorCode(errorCode) {}

    GLenum code() const { return errorCode; }

  private:
    GLenum errorCode;

    static std::string buildMessage(GLenum errorCode) {
        std::ostringstream oss;
        oss << "OpenGL Error: 0x" << std::hex << errorCode;
        return oss.str();
    }
};

// Controls
uint_fast16_t FORWARD_KEY = GLFW_KEY_W;
uint_fast16_t BACKWARD_KEY = GLFW_KEY_S;
uint_fast16_t RIGHT_KEY = GLFW_KEY_Q;
uint_fast16_t LEFT_KEY = GLFW_KEY_A;
uint_fast16_t JUMP_KEY = GLFW_KEY_SPACE;
uint_fast16_t UP_KEY = GLFW_KEY_LEFT_SHIFT;
uint_fast16_t SPRINT_KEY = GLFW_KEY_LEFT_CONTROL;
uint_fast16_t DOWN_KEY = GLFW_KEY_LEFT_CONTROL;
uint_fast16_t CROUCH_KEY = GLFW_KEY_LEFT_SHIFT;
uint_fast16_t MENU_KEY = GLFW_KEY_F10;
uint_fast16_t EXIT_KEY = GLFW_KEY_ESCAPE;
uint_fast16_t FULLSCREEN_KEY = GLFW_KEY_F11;
uint_fast16_t WIREFRAME_KEY = GLFW_KEY_0;
uint_fast16_t SURVIVAL_MODE_KEY = GLFW_KEY_1;
uint_fast16_t CREATIVE_MODE_KEY = GLFW_KEY_2;
uint_fast16_t SPECTATOR_MODE_KEY = GLFW_KEY_3;
uint_fast16_t CAMERA_SWITCH_KEY = GLFW_KEY_R;
uint_fast8_t ATTACK_BUTTON = GLFW_MOUSE_BUTTON_LEFT;
uint_fast8_t DEFENSE_BUTTON = GLFW_MOUSE_BUTTON_RIGHT;

// Paths
std::filesystem::path WORKING_DIRECTORY = std::filesystem::current_path();
// 	Shaders
std::filesystem::path SHADERS_DIRECTORY = WORKING_DIRECTORY / "shaders";

std::filesystem::path PLAYER_VERTEX_SHADER_DIRECTORY = SHADERS_DIRECTORY / "player_vert.glsl";
std::filesystem::path PLAYER_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "player_frag.glsl";

std::filesystem::path CHUNK_VERTEX_SHADER_DIRECTORY = SHADERS_DIRECTORY / "chunk_vert.glsl";
std::filesystem::path CHUNK_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "chunk_frag.glsl";

std::filesystem::path WATER_VERTEX_SHADER_DIRECTORY = SHADERS_DIRECTORY / "water_vert.glsl";
std::filesystem::path WATER_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "water_frag.glsl";

std::filesystem::path CROSSHAIR_VERTEX_SHADER_DIRECTORY = SHADERS_DIRECTORY / "crosshair_vert.glsl";
std::filesystem::path CROSSHAIR_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "crosshair_frag.glsl";

std::filesystem::path UI_VERTEX_SHADER_DIRECTORY = SHADERS_DIRECTORY / "ui_vert.glsl";
std::filesystem::path UI_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "ui_frag.glsl";
// 	Assets
std::filesystem::path ASSETS_DIRECTORY = WORKING_DIRECTORY / "assets";
std::filesystem::path UI_DIRECTORY = ASSETS_DIRECTORY / "UI";
std::filesystem::path WINDOW_ICON_DIRECTORY = ASSETS_DIRECTORY / "alpha.png";
std::filesystem::path BLOCK_ATLAS_TEXTURE_DIRECTORY = ASSETS_DIRECTORY / "block_map.png";
std::filesystem::path DEFAULT_SKIN_DIRECTORY = ASSETS_DIRECTORY / "Player/default_skin.png";
std::filesystem::path ICONS_DIRECTORY = UI_DIRECTORY / "icons.png";
std::filesystem::path FONTS_DIRECTORY = UI_DIRECTORY / "fonts";
std::filesystem::path MAIN_FONT_DIRECTORY = FONTS_DIRECTORY / "Hack-Regular.ttf";
std::filesystem::path MAIN_DOC_DIRECTORY = UI_DIRECTORY / "main.html";

// World Attributes
float GRAVITY = 9.80665f;

// Utils for rendering
unsigned int g_drawCallCount = 0;
void DrawArraysWrapper(GLenum mode, GLint first, GLsizei count) {
    glDrawArrays(mode, first, count);
    g_drawCallCount++;
#if defined(DEBUG)
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in DrawArraysWrapper: Error code: "
                  << std::hex << error << std::dec << std::endl;
    }
#endif
}

void DrawElementsWrapper(GLenum mode, GLsizei count, GLenum type, const void *indices) {
    glDrawElements(mode, count, type, indices);
    g_drawCallCount++;
#if defined(DEBUG)
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in DrawElementsWrapper: Error code: "
                  << std::hex << error << std::dec << std::endl;
    }
#endif
}

void DrawArraysInstancedWrapper(GLenum mode, GLint first, GLsizei count, GLsizei instanceCount) {
    glDrawArraysInstanced(mode, first, count, instanceCount);
    g_drawCallCount++;
#if defined(DEBUG)
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL Error in DrawArraysInstancedWrapper: Error code: "
                  << std::hex << error << std::dec << std::endl;
    }
#endif
}

float LINE_WIDTH = 1.5f;
bool WIREFRAME_MODE = false;
bool DEPTH_TEST = true;
uint8_t DEPTH_BITS = 24;
uint8_t STENCIL_BITS = 8;
bool BLENDING = true;
bool V_SYNC = false;
bool FACE_CULLING = true;
bool MSAA = false;
uint8_t MSAA_STRENGHT = 4; // 4x MSAA Means 4 samples per pixel. Common options are 2, 4, 8, or 16—higher numbers improve quality but increase GPU memory and computation cost.
