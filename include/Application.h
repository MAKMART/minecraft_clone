#pragma once
#include "defines.h"
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <windows.h>
#include <shellapi.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Player/Player.h"
#include "SurvivalMode.h"
#include "CreativeMode.h"
#include "SpectatorMode.h"
#include "WalkingState.h"
#include "FlyingState.h"
#include "RunningState.h"
#include "SwimmingState.h"
#include "InputManager.h"
#include <gl/GL.h>

class Application {
public:
    Application(int width, int height);
    ~Application(void);
    void Run(void);
private:
    GLFWwindow* window;
    std::string title;
    u16 width, height;
    float aspectRatio;
    glm::vec4 backgroundColor;

    u8 nbFrames = 0;
    f32 deltaTime = 0.0f;
    f32 lastFrame = 0.0f;

    b8 isFullscreen = false;
    b8 FREE_CURSOR = false;
    b8 mouseClickEnabled;
    b8 renderUI = true;
    b8 renderTerrain = true;

    i32 windowedWidth, windowedHeight;
    i32 windowedPosX, windowedPosY;

    b8 firstMouse = true;




    std::unique_ptr<Shader> playerShader;
    std::unique_ptr<ChunkManager> chunkManager;
    std::unique_ptr<Player> player;
    std::unique_ptr<InputManager> input;
    glm::ivec3 chunkSize = {16, 128, 16};	// Size of each chunk


    void initWindow(void);
    void handleFullscreenToggle(GLFWwindow* window);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    void processInput();
    float getFPS(float delta);

    // -- Crosshair ---
    std::unique_ptr<Shader> crossHairshader;
    std::unique_ptr<Texture> crossHairTexture;
    // Use std::vector for dynamic size safety
    std::vector<float> Crosshairvertices;
    static constexpr unsigned int CrosshairIndices[6] = {
        0, 1, 2,  // First triangle
        2, 3, 0   // Second triangle
    };
    float crosshairSize;

    unsigned int crosshairVAO, crosshairVBO, crosshairEBO;

    // Constants
    const int frametime_max = 100; // Store last 100 frame times
    std::vector<float> frametimes; // Declare without initialization here
    int frameIndex = 0;
    // Call this every frame
    void UpdateFrametimeGraph(float deltaTime)
    {
	// Convert deltaTime to milliseconds
	float frametimeMs = deltaTime * 1000.0f;

	//float smoothedFrametime = (frametimeMs + frametimes[(frameIndex - 1 + MAX_FRAMETIMES) % MAX_FRAMETIMES]) / 2.0f;

	// Store in circular buffer
	frametimes[frameIndex] = frametimeMs;
	frameIndex = (frameIndex + 1) % frametime_max; // Wrap around
    }
    // Render ImGui graph
    void RenderFrametimeGraph(void)
    {

	// Display the graph
	ImGui::PlotLines("##frametime", frametimes.data(), frametime_max, frameIndex, 
		"Frametime (ms)", 0.0f, 48.0f, ImVec2(0, 100));

	// Show latest frame time
	float lastFrametime = frametimes[(frameIndex - 1 + frametime_max) % frametime_max];
	ImGui::Text("Last Frametime: %.2f ms", lastFrametime);
	ImGui::Text("FPS: %.1f", 1000.0f / lastFrametime);

    }
    static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
};
