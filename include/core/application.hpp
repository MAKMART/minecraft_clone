#pragma once
#include "core/defines.hpp"
#include "core/window_context.hpp"
#include "game/player.hpp"
#include "game/ecs/components/input.hpp"
#include "graphics/shader.hpp"
#include "ui/ui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <cstdlib>
#include "core/timer.hpp"
#include "core/logger.hpp"
#include "chunk/chunk_manager.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/position.hpp"
#include "game/ecs/components/velocity.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/collider.hpp"
#include "game/ecs/components/input.hpp"
#include "game/ecs/components/player_mode.hpp"
#include "game/ecs/components/player_state.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/ecs/components/camera_controller.hpp"
#include "game/ecs/components/mouse_input.hpp"
#include "game/ecs/components/active_camera.hpp"

class Application
{
      public:
	Application(int width, int height);
	~Application();
	void Run();

      private:
	std::string title = std::string(PROJECT_NAME) + std::string(" ") + std::string(PROJECT_VERSION);
	u16         width, height;
	float       aspectRatio;
	glm::vec4   backgroundColor;

	u8  nbFrames  = 0;
	f32 deltaTime = 0.0f;
	f32 lastFrame = 0.0f;

	b8 isFullscreen  = false;
	b8 renderUI      = true;
	b8 renderTerrain = true;
#if defined(DEBUG)
	b8 debugRender = false;
#endif
	i32         windowedWidth, windowedHeight;
	i32         windowedPosX, windowedPosY;
	GLFWwindow* window;
	WindowContext* context;

	b8 firstMouse = true;

	ECS ecs;

	// Initialize in safe order
	std::unique_ptr<ChunkManager> chunkManager;
	std::unique_ptr<Player>       player;
	std::unique_ptr<Shader>       playerShader;
	std::unique_ptr<UI>           ui;

	GLFWwindow* createWindow();
	void        processInput();
	float       getFPS();
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

	// -- Crosshair ---
	std::unique_ptr<Shader>  crossHairshader;
	std::unique_ptr<Texture> crossHairTexture;
	// Use std::vector for dynamic size safety
	std::vector<float>            Crosshairvertices;
	static constexpr unsigned int CrosshairIndices[6] = {
	    0, 1, 2, // First triangle
	    2, 3, 0  // Second triangle
	};
	float crosshairSize;

	unsigned int crosshairVAO, crosshairVBO, crosshairEBO;

	static void MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
};
