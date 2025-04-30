#include "defines.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>


// Controls
uint_fast16_t FORWARD_KEY 		= GLFW_KEY_W;
uint_fast16_t BACKWARD_KEY 		= GLFW_KEY_S;
uint_fast16_t RIGHT_KEY 		= GLFW_KEY_D;
uint_fast16_t LEFT_KEY 			= GLFW_KEY_A;
uint_fast16_t JUMP_KEY 			= GLFW_KEY_SPACE;
uint_fast16_t UP_KEY			= GLFW_KEY_LEFT_SHIFT;
uint_fast16_t SPRINT_KEY 		= GLFW_KEY_LEFT_CONTROL;
uint_fast16_t DOWN_KEY			= GLFW_KEY_LEFT_CONTROL;
uint_fast16_t CROUCH_KEY 		= GLFW_KEY_LEFT_SHIFT;
uint_fast16_t MENU_KEY 			= GLFW_KEY_F10;
uint_fast16_t EXIT_KEY	 		= GLFW_KEY_ESCAPE;
uint_fast16_t FULLSCREEN_KEY 		= GLFW_KEY_F11;
uint_fast16_t WIREFRAME_KEY 		= GLFW_KEY_0;
uint_fast16_t SURVIVAL_MODE_KEY 	= GLFW_KEY_1;
uint_fast16_t CREATIVE_MODE_KEY 	= GLFW_KEY_2;
uint_fast16_t SPECTATOR_MODE_KEY 	= GLFW_KEY_3;
uint_fast16_t CAMERA_SWITCH_KEY		= GLFW_KEY_R;
uint_fast8_t  ATTACK_BUTTON 		= GLFW_MOUSE_BUTTON_LEFT;
uint_fast8_t  DEFENSE_BUTTON 		= GLFW_MOUSE_BUTTON_RIGHT;


// Paths
std::filesystem::path WORKING_DIRECTORY = std::filesystem::current_path();
// 	Shaders
std::filesystem::path SHADERS_DIRECTORY = WORKING_DIRECTORY / "shaders";
std::filesystem::path CHUNK_VERTEX_SHADER_DIRECTORY = SHADERS_DIRECTORY / "chunkvert.glsl";
std::filesystem::path CHUNK_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "chunkfrag.glsl";
std::filesystem::path CROSSHAIR_VERTEX_SHADER_DIRECTORY = SHADERS_DIRECTORY / "crosshairVert.glsl";
std::filesystem::path CROSSHAIR_FRAGMENT_SHADER_DIRECTORY = SHADERS_DIRECTORY / "crosshairFrag.glsl";
// 	Assets
std::filesystem::path ASSETS_DIRECTORY = WORKING_DIRECTORY / "assets";
std::filesystem::path WINDOW_ICON_DIRECTORY = ASSETS_DIRECTORY / "alpha.png";
std::filesystem::path BLOCK_ATLAS_TEXTURE_DIRECTORY = ASSETS_DIRECTORY / "block_map.png";
std::filesystem::path DEFAULT_SKIN_DIRECTORY = ASSETS_DIRECTORY / "Player/default_skin.png";
std::filesystem::path ICONS_DIRECTORY = ASSETS_DIRECTORY / "UI/icons.png";

// World Attributes
float GRAVITY = 9.80665f;


//Utils for rendering
unsigned int g_drawCallCount = 0;
void GLCheckError(const char* file, int line) {
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        std::cerr << RED << "OpenGL Error: " << std::hex << error << std::dec
          << " | File: " << file << " | Line: " << line << RESET_COLOR << std::endl;
    }
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

