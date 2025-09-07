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
#include "game/ecs/components/active_camera.hpp"

class Application
{
      public:
	Application(int width, int height, std::string title);
	~Application();
	void Run();

      private:
	glm::vec4 backgroundColor;

	u8  nbFrames  = 0;
	f32 deltaTime = 0.0f;
	f32 lastFrame = 0.0f;

	b8 renderUI      = true;
	b8 renderTerrain = true;
#if defined(DEBUG)
	b8 debugRender = false;
#endif
	std::unique_ptr<WindowContext> context;
	std::unique_ptr<ChunkManager>  chunkManager;
	std::unique_ptr<Player>        player;
	std::unique_ptr<Shader>        playerShader;
	std::unique_ptr<UI>            ui;
	ECS                            ecs;

	void        processInput();
	float       getFPS();
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

	// -- Crosshair ---
	std::unique_ptr<Shader>  crossHairshader;
	std::unique_ptr<Texture> crossHairTexture;
	std::vector<float>            Crosshairvertices;
	static constexpr unsigned int CrosshairIndices[6] = {
	    0, 1, 2,
	    2, 3, 0
	};
	float crosshairSize;

	unsigned int crosshairVAO, crosshairVBO, crosshairEBO;
};
