
/*
 *
 * TODO: IMPLEMENT THE APPSTATE AND THE MAIN MENU WITH THE NEW UI SYSTEM
 */


#if defined(TRACY_ENABLE)
#include "tracy/Tracy.hpp"
#endif

#include <stb_image.h>
#include <gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cassert>

import std;
import timer;
import core;
import frame_context;
import gl_state;
import input_manager;
import raycast;
import logger;
import shader;
import texture;
import ui;
import ecs;
import ecs_components;
import ecs_systems;
import framebuffer_manager;
import chunk_manager;
import glm;
import window_context;
import player;
import debug_drawer;
import aabb;
import vertex_buffer;
import index_buffer;
import app_state;
import debug;

static float crosshair_size = 0.3f;
float getFPS(const frame_context& ctx);
void DrawBool(const char* label, bool value);

const char* gl_enum_to_string(GLenum e) {
	switch (e) {
		case GL_RGBA8:               return "GL_RGBA8";
		case GL_RGBA16F:             return "GL_RGBA16F";
		case GL_RGB16F:              return "GL_RGB16F";
		case GL_DEPTH_COMPONENT24:   return "GL_DEPTH_COMPONENT24";
		case GL_DEPTH_COMPONENT32F:  return "GL_DEPTH_COMPONENT32F";
		case GL_DEPTH24_STENCIL8:    return "GL_DEPTH24_STENCIL8";
		default:                     return "UNKNOWN_GL_ENUM";
	}
}
int main()
{
  app_state state(1920, 1080, MAIN_FONT_DIRECTORY);
  glfwSetWindowUserPointer(state.win_context->window, &state);
	InputManager::get().setContext(state.win_context.get());
	InputManager::get().setMouseTrackingEnabled(true);
	auto framebuffer_size_callback_lambda = [](GLFWwindow* window, int width, int height) {
		// Don't recalculate the projection matrix, skip this frame's rendering, or log a warning
		if (width <= 0 || height <= 0) return;
		log::info("width = {}, height = {}", width, height);

    app_state *state = static_cast<app_state*>(glfwGetWindowUserPointer(window));
    if (!state) log::error("Couldn't find app_state* with glfwGetWindowUserPointer(window)");

		glfwGetFramebufferSize(window, &state->frame_ctx.fb_width, &state->frame_ctx.fb_height);
		log::info("fb_width = {}, fb_height = {}", state->frame_ctx.fb_width, state->frame_ctx.fb_height);
    GLState::set_viewport(0, 0, width, height);
		ImGui::SetNextWindowPos(ImVec2(width/* - 300*/, height), ImGuiCond_Always);
    assert(state->frame_ctx.active_camera.is_valid());
		state->g_state.ecs.get_component<Camera>(state->frame_ctx.active_camera)->aspect_ratio = float(static_cast<float>(state->frame_ctx.fb_width) / state->frame_ctx.fb_height);

    state->g_state.ecs.for_each_component<RenderTarget>([&](const Entity e, RenderTarget& rt) {
        /*
        switch (rt.mode) {
          case extent_mode::follow_viewport:
            rt.width = state->frame_ctx.fb_width;
            rt.height = state->frame_ctx.fb_height;
            break;
          case extent_mode::fixed:
          // Dont make rt.width && rt.height follow the viewport
          default:
            break;
        }
        */
        state->fb_manager.ensure(e, rt);
				log::info("rt.width = {}, rt.height = {}", rt.width, rt.height);
		});
    state->ui.SetViewportSize(state->frame_ctx.fb_width, state->frame_ctx.fb_height);
	};
	glfwSetFramebufferSizeCallback(state.win_context->window, framebuffer_size_callback_lambda);

	// TODO: fix the damn stencil buffer to allow cropping in RmlUI

	// TODO:    FIX THE CAMERA CONTROLLER TO WORK WITH INTERPOLATION

	/*
	int nExt;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nExt);
	for (int i = 0; i < nExt; ++i) {
		std::cout << glGetStringi(GL_EXTENSIONS, i) << std::endl;
	}
	*/
	// int fb_width, fb_height;
	// glfwGetFramebufferSize(context->window, &fb_width, &fb_height);

	log::info("Initializing Shaders...");

	Shader playerShader("Player", PLAYER_VERTEX_SHADER_DIRECTORY, PLAYER_FRAGMENT_SHADER_DIRECTORY);
	Shader fb_debug("FB_DEBUG", SHADERS_DIRECTORY / "fb_vert.glsl", SHADERS_DIRECTORY / "fb_debug_frag.glsl");
	Shader fb_player("FB_PLAYER", SHADERS_DIRECTORY / "fb_vert.glsl", SHADERS_DIRECTORY / "fb_player_frag.glsl");
	Shader shad("Test", SHADERS_DIRECTORY / "test_vert.glsl", SHADERS_DIRECTORY / "test_frag.glsl");

	ChunkManager manager;
	manager.generate_chunks({0.0f, 0.0f, 0.0f}, 5u);
  Texture Atlas(BLOCK_ATLAS_TEXTURE_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);

	// Cubemap
	GLuint cube_id;
	glGenTextures(1, &cube_id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cube_id);

	std::array<std::string, 6> cube_faces = {
		/*
		"skybox/right.jpg", // +x
		"skybox/left.jpg",	// -x
		"skybox/bottom.jpg",// -y
		"skybox/top.jpg",	// +y
		"skybox/front.jpg",	// +z
		"skybox/back.jpg"	// -z
		*/
		"skybox/px.png",
		"skybox/nx.png",
		"skybox/ny.png",
		"skybox/py.png",
		"skybox/pz.png",
		"skybox/nz.png"
	};

	int width, height, nrChannels;
	unsigned char *data;
	for(unsigned int i = 0; i < cube_faces.size(); i++)
	{
		std::string string = ASSETS_DIRECTORY / cube_faces[i];
		//log::info("loading cubemap texture from: {}", string);
		data = stbi_load(string.c_str(), &width, &height, &nrChannels, 0);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
					width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); // BEWARE OF THE FORMAT: It has to match the provided images
			stbi_image_free(data);
		} else {
			log::error("Failed to load skybox: {}", string);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


	// Initialize framebuffers for render targets
	state.g_state.ecs.for_each_component<RenderTarget>([&](Entity e, RenderTarget& rt) {
			state.fb_manager.ensure(e, rt);
			/*
			for(size_t i = 0; i < rt.attachments.size(); i++) {
			unsigned int e = (unsigned int)rt.attachments[i].internal_format;
			std::string s;
			switch (e) {
			case 0x8058:    s = "GL_RGBA8"; break;
			case 0x881A:    s = "GL_RGBA16F"; break;
			case 0x881B:    s = "GL_RGB16F"; break;
			case 0x822F:	s = "GL_RG16F"; break;
			case 0x81A6:	s = "GL_DEPTH_COMPONENT24"; break;
			case 0x8CAC:	s = "GL_DEPTH_COMPONENT32F"; break;
			case 0x88F0:    s = "GL_DEPTH24_STENCIL8"; break;
			default:        s = "UNKNOWN_GL_ENUM"; break;
			}
			log::info("rt.attachments[{}].internal_format = {}", i, s);
			}
			std::cout << "\n";
			*/
	});
	// --- CROSSHAIR STUFF ---
	Shader crossHairshader("Crosshair", CROSSHAIR_VERTEX_SHADER_DIRECTORY, CROSSHAIR_FRAGMENT_SHADER_DIRECTORY);
	Texture crossHairTexture(ICONS_DIRECTORY, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);


	static constexpr int CrosshairIndices[6] = {
	    0, 1, 2,
	    2, 3, 0
	};
	float crosshairSize;
	unsigned int crosshairVAO;
	//std::cout << crossHairTexture.getWidth() << " x " << crossHairTexture.getHeight() << "\n";
	const float textureWidth  = 512.0f;
	const float textureHeight = 512.0f;
	const float cellWidth     = 15.0f;
	const float cellHeight    = 15.0f;
	// Define row and column (0-based index, top-left starts at row=0, col=0)
	const int row = 0; // First row (from the top)
	const int col = 0; // First column
	float uMin = (col * cellWidth) / textureWidth;
	float vMin = 1.0f - ((row + 1) * cellHeight) / textureHeight;
	float uMax = ((col + 1) * cellWidth) / textureWidth;
	float vMax = 1.0f - (row * cellHeight) / textureHeight;
	crosshairSize = std::floor(cellWidth * 3.5f);

	float verts[] = {
		-crosshairSize, -crosshairSize, 0.0f, uMin, vMin,
		crosshairSize, -crosshairSize, 0.0f, uMax, vMin,
		crosshairSize,  crosshairSize, 0.0f, uMax, vMax,
		-crosshairSize,  crosshairSize, 0.0f, uMin, vMax
	};


	glCreateVertexArrays(1, &crosshairVAO);
	vertex_buffer_immutable crosshairVBO = vertex_buffer_immutable(verts, sizeof(verts));
	index_buffer_immutable<std::int32_t> crosshairEBO = index_buffer_immutable<std::int32_t>(CrosshairIndices, std::size(CrosshairIndices)); // std::size(CrosshairIndices) == 6

	glVertexArrayVertexBuffer(crosshairVAO, 0, crosshairVBO.id(), 0, 5 * sizeof(float));
	glVertexArrayElementBuffer(crosshairVAO, crosshairEBO.id());

	glVertexArrayAttribFormat(crosshairVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(crosshairVAO, 0, 0);
	glEnableVertexArrayAttrib(crosshairVAO, 0);

	glVertexArrayAttribFormat(crosshairVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
	glVertexArrayAttribBinding(crosshairVAO, 1, 0);
	glEnableVertexArrayAttrib(crosshairVAO, 1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(state.win_context->window, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	GLuint VAO;
	glCreateVertexArrays(1, &VAO);
	// Position attribute (location 0)
	glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(VAO, 0, 0);
	glEnableVertexArrayAttrib(VAO, 0);

	// UV attribute (location 1)
	glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
	glVertexArrayAttribBinding(VAO, 1, 0);
	glEnableVertexArrayAttrib(VAO, 1);

	float vertices[] = {
		// positions       // uvs
		-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
		3.0f, -1.0f, 0.0f,  2.0f, 0.0f,
		-1.0f,  3.0f, 0.0f,  0.0f, 2.0f
	};

	vertex_buffer_immutable vb = vertex_buffer_immutable(vertices, sizeof(vertices));
	glVertexArrayVertexBuffer(
			VAO,
			0,                  // binding index
			vb.id(),
			0,                  // offset in buffer
			5 * sizeof(float)   // stride
			);






















	double lastFrame = glfwGetTime();
















	while (!state.win_context->should_close()) {	// GAME LOOP
		double currentFrame = glfwGetTime();
    state.frame_ctx.delta_time = currentFrame - lastFrame;
    state.frame_ctx.first_frame = state.frame_ctx.frame_number == 0;  // The fist frame has idx 0x0000000
		lastFrame          = currentFrame;
		g_drawCallCount    = 0;
		// Clear the screen for the start of the new current frame
		glClearDepth(0.0f);
		GLState::set_depth_test(true);
		glStencilMask(0xFF); // Ensure we can actually clear the stencil bits
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


		// UPDATE PHASE

		auto& input = InputManager::get();

		UpdateFrametimeGraph(state.frame_ctx.delta_time);
		// --- Input & Event Processing ---
		glfwPollEvents();     // new events arrive → PRESSED/RELEASED, mouse, scroll
		input_system(state.g_state.ecs);



    // state.frame_ctx.active_camera = state.g_state.camera_manager_entity;
    state.frame_ctx.active_camera = state.g_state.get_active_camera_component()->target;

		Camera* ________cam = state.g_state.ecs.get_component<Camera>(state.frame_ctx.active_camera);
		if (!________cam )
			log::error("Couldn't find active cam for frame {}", state.frame_ctx.frame_number);
    state.frame_ctx.cam = ________cam;
    glm::mat4 view_proj = state.frame_ctx.cam->viewMatrix * state.frame_ctx.cam->projectionMatrix;
    state.frame_ctx.view_proj_matrix = view_proj;
    state.frame_ctx.inv_view_proj_matrix = glm::inverse(view_proj);


    { // Switch currently active camera
      auto* active_camera = state.g_state.get_active_camera_component();

      if (input.isPressed(GLFW_KEY_RIGHT)) {
        active_camera->target = state.g_state.player.camera;
        state.g_state.ecs.add_component_if_missing<InputComponent>(state.g_state.player.self, InputComponent{});
      }

      if (input.isPressed(GLFW_KEY_LEFT)) {
#if defined(DEBUG)
        active_camera->target = state.g_state.debug_cam;
        state.g_state.ecs.add_component_if_missing<InputComponent>(state.g_state.debug_cam, InputComponent{});
#endif
      }

    }



		if (input.isPressed(EXIT_KEY))
			glfwSetWindowShouldClose(state.win_context->window, true);

		if (input.isPressed(FULLSCREEN_KEY)) {
			state.win_context->toggle_fullscreen();
			int fb_w, fb_h;
			glfwGetFramebufferSize(state.win_context->window, &fb_w, &fb_h);
			framebuffer_size_callback_lambda(state.win_context->window, fb_w, fb_h);
		}

		static float scale = 0.01f;
		if (state.frame_ctx.active_camera == state.g_state.player.camera) {
			//fb_player.setFloat("near_plane", cam->near_plane);
			//fb_player.setFloat("far_plane", cam->far_plane);
			fb_player.setFloat("scale", scale);
		} 
#if defined(DEBUG)
		else if (state.frame_ctx.active_camera == state.g_state.debug_cam) {
			fb_debug.setFloat("near_plane", state.frame_ctx.cam->near_plane);
			fb_debug.setFloat("far_plane", state.frame_ctx.cam->far_plane);
		}
#endif
		if (input.isPressed(GLFW_KEY_UP))
			scale+=0.01f;
		if (input.isPressed(GLFW_KEY_DOWN))
			scale-=0.01f;

		static bool toggle = false;
		static bool was_pressed = false;

		bool pressed = input.isPressed(GLFW_KEY_6);

		if (pressed && !was_pressed) { // only trigger on key-down edge
			toggle = !toggle;          // flip toggle
			if (state.frame_ctx.active_camera == state.g_state.player.camera) {
				fb_player.setInt("toggle", toggle ? 1 : 0);
			} 
#if defined(DEBUG)
			else if (state.frame_ctx.active_camera == state.g_state.debug_cam) {
				fb_debug.setInt("toggle", toggle ? 1 : 0);
			}
#endif
		}

		was_pressed = pressed; // remember state for next frame

		// 1. Player
		if (input.isMouseTrackingEnabled()) {
			// if (input.isMousePressed(ATTACK_BUTTON) && state.g_state.player.canBreakBlocks && state.frame_ctx.active_camera == state.g_state.player.camera) {
			// 	state.g_state.player.breakBlock(manager);
			// }
			// if (input.isMousePressed(DEFENSE_BUTTON) && state.g_state.player.canPlaceBlocks && state.frame_ctx.active_camera == state.g_state.player.camera) {
			// 	state.g_state.player.placeBlock(manager);
			// }
#if defined(DEBUG)
			if (state.frame_ctx.active_camera == state.g_state.debug_cam) {
				Transform* trans = state.g_state.ecs.get_component<Transform>(state.frame_ctx.active_camera);
				if (!trans)
					log::error("NO TRANSFORM FOR ENTITY: {}", state.frame_ctx.active_camera.id);
				glm::vec4 clip = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

				glm::vec4 view_space = glm::inverse(state.frame_ctx.cam->projectionMatrix) * clip;
				view_space.z = -1.0f; // forward direction
				view_space.w = 0.0f;  // this is a direction, not a position

				glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(state.frame_ctx.cam->viewMatrix) * view_space));
				glm::vec3 ray_origin = trans->pos; // start at camera position
				if (input.isMouseHeld(ATTACK_BUTTON)) {
					std::optional<glm::ivec3> hitBlock = raycast::voxel(manager, ray_origin, ray_dir, state.g_state.player.max_interaction_distance);
					if (hitBlock.has_value()) {
						glm::ivec3 blockPos = hitBlock.value();
						manager.update_block(blockPos, Block::blocks::AIR);
					}
					// log::info("Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);
				}
				if (input.isMouseHeld(DEFENSE_BUTTON)) {
					std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycast::voxel_normals(manager, ray_origin, ray_dir, state.g_state.player.max_interaction_distance);
					if (hitResult.has_value()) {
						glm::ivec3 hitBlockPos = hitResult->first;
						glm::ivec3 normal      = hitResult->second;
						glm::ivec3 placePos = hitBlockPos + (-normal);
						glm::ivec3 localBlockPos = world_to_local(placePos);

						if (manager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z) != Block::blocks::AIR) {
							log::error("❌ Target block is NOT air! It's type: {}", Block::toString(manager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z)));
						}

						//log::info("Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);
						/*
						int radius = 2;
						for(int i = -radius; i < radius; i++) {
							for(int j = -radius; j < radius; j++) {
								for(int k = -radius; k < radius; k++)
									manager.updateBlock(placePos + glm::ivec3(i, j, k), static_cast<Block::blocks>(state.g_state.player.selectedBlock));
							}
						}
						*/
						manager.update_block(placePos, static_cast<Block::blocks>(state.g_state.player.selectedBlock));
					}
				}
			
			}
#endif


			float scrollY = input.getScroll().y;
			if (scrollY != 0.0f) {
				CameraController* ctrl = state.g_state.ecs.get_component<CameraController>(state.frame_ctx.active_camera);
				float scroll_speed_multiplier = 1.0f;
				if (!ctrl->third_person) {
					state.g_state.player.selectedBlock += static_cast<int>(scrollY * scroll_speed_multiplier);
					if (state.g_state.player.selectedBlock < 1)
						state.g_state.player.selectedBlock = 1;
					if (state.g_state.player.selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS))
						state.g_state.player.selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
				}

				// To change FOV
				// camCtrl.processMouseScroll(scrollY);


			}
		}

		PlayerMode* mode = state.g_state.ecs.get_component<PlayerMode>(state.g_state.player.self);
		PlayerState* player_state = state.g_state.ecs.get_component<PlayerState>(state.g_state.player.self);
		if (input.isPressed(CAMERA_SWITCH_KEY) && state.g_state.ecs.has_component<CameraController>(state.frame_ctx.active_camera)) {
			state.g_state.ecs.get_component<CameraController>(state.frame_ctx.active_camera)->third_person = !state.g_state.ecs.get_component<CameraController>(state.frame_ctx.active_camera)->third_person;
		}

		if (input.isPressed(SURVIVAL_MODE_KEY)) {
			player_state->current = PlayerMovementState::Walking;
		}

		if (input.isPressed(CREATIVE_MODE_KEY)) {
			player_state->current = PlayerMovementState::Flying;
		}

		if (input.isPressed(SPECTATOR_MODE_KEY)) {
			mode->mode = Type::SPECTATOR;
		}

		// Reload shaders
		if (input.isPressed(GLFW_KEY_H)) {
			manager.getShader().reload();
			playerShader.reload();
			fb_debug.reload();
			fb_player.reload();
			shad.reload();
			playerShader.reload();
			crossHairshader.reload();
		}

		if (input.isHeld(GLFW_KEY_P))
			crosshair_size += 0.1f;
		if (input.isHeld(GLFW_KEY_M))
			crosshair_size -= 0.1f;

#if defined(DEBUG)
		if (input.isPressed(GLFW_KEY_ENTER) && state.frame_ctx.active_camera == state.g_state.debug_cam) {
			state.g_state.ecs.get_component<Transform>(state.g_state.player.self)->pos = state.g_state.ecs.get_component<Transform>(state.g_state.debug_cam)->pos;
			state.g_state.ecs.get_component<Velocity>(state.g_state.player.self)->value = state.g_state.ecs.get_component<Velocity>(state.g_state.debug_cam)->value;
		
		}
#endif
		
		

		// 2. UI
		if (state.ui.get_context()) {
			glm::vec2 mousePos = input.getMousePos();
			state.ui.get_context()->ProcessMouseMove(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y), state.ui.GetKeyModifiers());

			// Mouse buttons
			for (int b = 0; b < InputManager::MAX_MOUSE_BUTTONS; ++b) {
				if (input.isMousePressed(b))
					state.ui.get_context()->ProcessMouseButtonDown(b, 0);
				else if (input.isMouseReleased(b))
					state.ui.get_context()->ProcessMouseButtonUp(b, 0);
			}

			// Scroll
			float scrollY = input.getScroll().y;
			if (scrollY != 0.0f)
				state.ui.get_context()->ProcessMouseWheel(-scrollY, 0);

			// Keyboard
			for (int k = 0; k < InputManager::MAX_KEYS; ++k) {
				if (input.isPressed(k))
					state.ui.get_context()->ProcessKeyDown(state.ui.MapKey(k), state.ui.GetKeyModifiers());
				else if (input.isReleased(k))
					state.ui.get_context()->ProcessKeyUp(state.ui.MapKey(k), state.ui.GetKeyModifiers());
			}
		}

#if defined(DEBUG)
		if (input.isPressed(GLFW_KEY_8)) {
			state.frame_ctx.debug_render = !state.frame_ctx.debug_render;
		}
#endif

		if (input.isPressed(MENU_KEY)) {
#if defined(DEBUG)
			UI::set_debugger_visible(input.isMouseTrackingEnabled());
#endif
			input.setMouseTrackingEnabled(!input.isMouseTrackingEnabled());
		}

#if defined(DEBUG)
		static bool _was_pressed = false;

		bool _pressed = input.isPressed(WIREFRAME_KEY);

		if (_pressed && !_was_pressed) { // only trigger on key-down edge
			GLState::set_wireframe(!GLState::is_wireframe());
		}

		_was_pressed = _pressed; // remember state for next frame

#endif




		CameraController* ctrl = state.g_state.ecs.get_component<CameraController>(state.g_state.player.camera);

		movement_intent_system(state.g_state.ecs, state.frame_ctx.cam);
    player_state_system(state.g_state.ecs);
		// movement_physics_system(state.g_state.ecs, manager, deltaTime);
		camera_controller_system(state.g_state.ecs, state.g_state.player.self);
#if defined(DEBUG)
		debug_camera_system(state.g_state.ecs, state.frame_ctx.delta_time);
#endif
		camera_system(state.g_state.ecs);
		frustum_volume_system(state.g_state.ecs);


#if defined(DEBUG)
		if (state.frame_ctx.debug_render) {
      auto aabb = state.g_state.ecs.get_component<Collider>(state.g_state.player.self)->aabb;
			DebugDrawer::get().addAABB(aabb, glm::vec3(0.3f, 1.0f, 0.5f));
			Camera* player_cam = state.g_state.ecs.get_component<Camera>(state.g_state.player.camera);
			Transform* player_cam_trans = state.g_state.ecs.get_component<Transform>(state.g_state.player.camera);
			glm::vec3 world_forward = glm::normalize(player_cam_trans->rot * player_cam->forward);
			glm::vec3 world_up      = glm::normalize(player_cam_trans->rot * player_cam->up);
			glm::vec3 world_right   = glm::normalize(player_cam_trans->rot * player_cam->right);
      auto* trans = state.g_state.ecs.get_component<Transform>(state.g_state.player.self);
			DebugDrawer::get().addRay(trans->pos, world_forward, {1.0f, 0.0f, 0.0f});
			DebugDrawer::get().addRay(trans->pos, world_up, {0.0f, 1.0f, 0.0f});
			DebugDrawer::get().addRay(trans->pos, world_right, {0.0, 0.0f, 1.0f});

			state.g_state.ecs.for_each_components<Camera, Transform>([&state](Entity e, Camera, Transform& trans){
					if (!state.g_state.ecs.has_component<ActiveCamera>(e))
						DebugDrawer::get().addAABB(AABB::fromCenterSize(trans.pos, {0.5f, 0.8f, 0.5f}), glm::vec3(1.0f, 0.0f, 1.0f));
					});

			float ndc_z = -1.0f; // near plane
			glm::vec4 clip = glm::vec4(0.0f, 0.0f, ndc_z, 1.0f);

			glm::vec4 view_space = glm::inverse(player_cam->projectionMatrix) * clip;
			view_space.z = -1.0f; // forward direction
			view_space.w = 0.0f;  // this is a direction, not a position

			glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(player_cam->viewMatrix) * view_space));
			glm::vec3 cam_pos   = player_cam_trans->pos + glm::vec3(0.1, 0.0, 0.1f);
			DebugDrawer::get().addRay(cam_pos, ray_dir, {1.0f, 0.0f, 0.0f});
			glm::vec3 cam_dir   = glm::normalize(player_cam_trans->rot * state.frame_ctx.cam->forward);
			DebugDrawer::get().addRay(cam_pos, cam_dir, {1.0f, 0.0f, 0.0f});


			// Add all chunks' bounding boxes
			for (const auto& [chunkKey, chunkPtr] : manager.get_all()) {
				if (!chunkPtr)
					continue; // safety

				// Color for chunk boxes, maybe a translucent blue-ish?
				DebugDrawer::get().addAABB(chunkPtr->aabb, glm::vec3(0.3f, 0.5f, 1.0f));
			}
      auto vel = state.g_state.ecs.get_component<Velocity>(state.g_state.player.self)->value;
			DebugDrawer::get().addRay(trans->pos, vel, {1.0f, 1.0f, 0.0f});
			DebugDrawer::get().addRay(trans->pos, glm::normalize(glm::vec3{0.0f, -GRAVITY, 0.0f}), glm::vec3(0.5f, 0.5f, 1.0f));
		}
#endif



		manager.generate_chunks(state.g_state.ecs.get_component<Transform>(state.g_state.player.self)->pos, state.g_state.player.render_distance);




		// RENDER PHASE

		// -- Render Player -- (BEFORE UI pass)
		// TODO: Actually fix and implement this shit
		/*
		if (player.renderSkin) {
			playerShader.setMat4("projection", cam->projectionMatrix);
			playerShader.setMat4("view", cam->viewMatrix);
			player.render(playerShader);
		}
    */


		if (state.g_state.render_terrain) {
			//manager.getShader().checkAndReloadIfModified();
			manager.getShader().setMat4("projection", state.frame_ctx.cam->projectionMatrix);
			manager.getShader().setMat4("view", state.frame_ctx.cam->viewMatrix);
			//manager.getShader().setFloat("time", (float)glfwGetTime());

			manager.getShader2().setMat4("u_projection", state.frame_ctx.cam->projectionMatrix);
			manager.getShader2().setMat4("u_view", state.frame_ctx.cam->viewMatrix);
			Transform* cam_trans = state.g_state.ecs.get_component<Transform>(state.frame_ctx.active_camera);
			manager.getShader2().setVec3("eye_position", cam_trans->pos);

			// manager.getShader2().setIVec3("eye_position_int", glm::ivec3(floor(cam_trans->pos)));
			Atlas.Bind(0);

			manager.getShader2().use();
			// Whatever VAO'll work
			glBindVertexArray(VAO);
			chunk_renderer_system(state.g_state.ecs, manager, state.frame_ctx.active_camera, *state.g_state.ecs.get_component<FrustumVolume>(state.g_state.player.camera), state.fb_manager);
			glBindVertexArray(0);
			Atlas.Unbind(0);
		}

		// Debug render

#if defined(DEBUG)
		if (state.frame_ctx.debug_render) {
			state.g_state.ecs.for_each_components<Camera, Transform, RenderTarget, ActiveCamera>([](Entity e, Camera& cam, Transform, RenderTarget, ActiveCamera){
					glm::mat4 pv = cam.projectionMatrix * cam.viewMatrix;
					DebugDrawer::get().draw(pv);
					});
		}
#endif

		glm::mat4 temp_model = glm::mat4(1.0f);
		temp_model = glm::translate(temp_model, glm::vec3(-0.5f, 9.5f, 0.0f));
		temp_model = glm::scale(temp_model, glm::vec3(0.5f, 0.5f, 0.5f));
		shad.setMat4("projection", state.frame_ctx.cam->projectionMatrix);
		shad.setMat4("model", temp_model);
		shad.setMat4("view", state.frame_ctx.cam->viewMatrix);

		Atlas.Bind(0);
		shad.use();
		glBindVertexArray(VAO);
		DrawArraysWrapper(GL_TRIANGLES, 0, 3);
		Atlas.Unbind(0);



		framebuffer::bind_default_draw();
		GLState::set_depth_test(false);
		auto& cur_fb = state.fb_manager.get(state.frame_ctx.active_camera);
		GLState::set_viewport(0, 0, cur_fb.width(), cur_fb.height());
		glBindTextureUnit(0, cur_fb.color_attachment(0));
		glBindTextureUnit(1, cur_fb.depth_attachment());
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cube_id);

		if (state.frame_ctx.active_camera == state.g_state.player.camera) {
			glm::mat4 prev_view_proj = state.g_state.ecs.get_component<CameraTemporal>(state.g_state.player.camera)->prev_view_proj;
			fb_player.setMat4("curr_inv_view_proj", state.frame_ctx.inv_view_proj_matrix);
			fb_player.setMat4("prev_view_proj", prev_view_proj);
			fb_player.setMat4("invView", glm::inverse(state.frame_ctx.cam->viewMatrix));
			fb_player.setMat4("invProj", glm::inverse(state.frame_ctx.cam->projectionMatrix));
			fb_player.setInt("color", 0);
			fb_player.setInt("depth", 1);
			fb_player.setInt("skybox", 2);
			fb_player.use();
		} 
#if defined(DEBUG)
		else if (state.frame_ctx.active_camera == state.g_state.debug_cam) {
			fb_debug.setInt("color", 0);
			fb_debug.setInt("depth", 1);
			fb_debug.use();
		}
#endif

		// NOTE: Make sure to have a VAO bound when making this draw call!!
		// Fullscreen triangle covering the whole screen for post-processing and shit like that
		GLState::set_wireframe(false);
		DrawArraysWrapper(GL_TRIANGLES, 0, 3);

		glBindVertexArray(0);

		// --- UI Pass --- (now rendered BEFORE ImGui)
		if (state.g_state.render_ui/* && state.frame_ctx.active_camera == player.camera*/) {
			GLState::set_depth_test(false);
			GLState::set_wireframe(false);
			GLState::set_stencil_test(false);
			// -- Crosshair Pass ---
			glm::mat4 orthoProj = glm::ortho(0.0f, static_cast<float>(cur_fb.width()), 0.0f, static_cast<float>(cur_fb.height()));
			crossHairshader.setMat4("uProjection", orthoProj);
			crossHairshader.setVec2("uCenter", glm::vec2(cur_fb.width() * 0.5f, cur_fb.height() * 0.5f));
			crossHairshader.setFloat("uSize", crosshair_size);

			crossHairTexture.Bind(2); // INFO: MAKE SURE TO BIND IT TO THE CORRECT TEXTURE BINDING!!!
			crossHairshader.use();
			glBindVertexArray(crosshairVAO);
			DrawElementsWrapper(GL_TRIANGLES, sizeof(CrosshairIndices) / sizeof(CrosshairIndices[0]), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			if (state.frame_ctx.active_camera == state.g_state.player.camera)
				state.ui.render();
			// other UI stuff...
		}
#if defined(NDEBUG) // TEMP
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("INFO", NULL,
		             ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize |
		                 ImGuiWindowFlags_::ImGuiWindowFlags_NoMove |
		                 ImGuiWindowFlags_::ImGuiWindowFlags_NoNavInputs |
		                 ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus |
		                 ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);
		ImGui::Text("FPS: %f", getFPS(state.frame_ctx));
		ImGui::Text("Selected block: ");
		ImGui::SameLine();
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(Block::toString(static_cast<Block::blocks>(state.g_state.player.selectedBlock)));
		ImGui::SetWindowFontScale(1.0f);
		ImGui::SliderInt("Render distance", (int*)&state.g_state.player.render_distance, 0, 30);
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

#if defined(DEBUG)
		if (state.g_state.render_ui) {
			// --- ImGui Debug UI Pass ---
			GLState::set_depth_test(false);
			GLState::set_wireframe(false);
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
      auto pos = state.g_state.ecs.get_component<Transform>(state.g_state.player.self)->pos;
      auto vel = state.g_state.ecs.get_component<Velocity>(state.g_state.player.self)->value;
			glm::ivec3 chunkCoords = world_to_chunk(pos);
			glm::ivec3 localCoords = world_to_local(pos);
			ImGui::Begin("DEBUG", NULL,
			             ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize |
			                 ImGuiWindowFlags_::ImGuiWindowFlags_NoMove |
			                 ImGuiWindowFlags_::ImGuiWindowFlags_NoNavInputs |
			                 ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus |
			                 ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::Selectable("Rendering Infos:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			ImGui::Text("FPS: %f", getFPS(state.frame_ctx));
			ImGui::Text("Draw Calls: %d", g_drawCallCount);
			RenderTimings();
			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::Selectable("Player:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			ImGui::Text("Player position: %f, %f, %f", pos.x, pos.y, pos.z);
			ImGui::Text("Player is in chunk: %i, %i, %i", chunkCoords.x, chunkCoords.y, chunkCoords.z);
			ImGui::Text("Player local position: %d, %d, %d", localCoords.x, localCoords.y, localCoords.z);
			ImGui::Text("Player velocity: %f, %f, %f", vel.x, vel.y, vel.z);
      ImGui::Text("Player STATE: %s", player_mode_to_string(*state.g_state.ecs.get_component<PlayerMode>(state.g_state.player.self)).data());
      ImGui::Text("Player STATE: %s", player_state_to_string(*state.g_state.ecs.get_component<PlayerState>(state.g_state.player.self)).data());
			// glm::vec3 _min = player.getAABB().min;
			// glm::vec3 _max = player.getAABB().max;
			// ImGui::Text("Player AABB { %f, %f, %f }, { %f, %f, %f }", _min.x, _min.y, _min.z, _max.x, _max.y, _max.z);
			ImGui::Text("Selected block: ");
			ImGui::SameLine();
			ImGui::SetWindowFontScale(1.2f);
			ImGui::Text("%s", Block::toString(static_cast<Block::blocks>(state.g_state.player.selectedBlock)));
			ImGui::SetWindowFontScale(1.0f);
			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::Selectable("Player flags:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			DrawBool("is OnGround", state.g_state.ecs.get_component<Collider>(state.g_state.player.self)->is_on_ground);
			DrawBool("is Damageable", state.g_state.player.isDamageable);
			DrawBool("is Running", player_state->current == PlayerMovementState::Running);
			DrawBool("is Flying", player_state->current == PlayerMovementState::Flying);
			DrawBool("is Swimming", player_state->current == PlayerMovementState::Swimming);
			DrawBool("is Walking", player_state->current == PlayerMovementState::Walking);
			DrawBool("is Crouched", player_state->current == PlayerMovementState::Crouching);
			DrawBool("is third-person", ctrl->third_person);
			DrawBool("Player can place blocks", state.g_state.player.canPlaceBlocks);
			DrawBool("Player can break blocks", state.g_state.player.canBreakBlocks);
			DrawBool("renderSkin", state.g_state.player.renderSkin);
			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
			ImGui::Selectable("Camera:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			// DrawBool("is Camera interpolating", player.getCameraController().isInterpolationEnabled());
			glm::vec3& camPos = state.g_state.ecs.get_component<Transform>(state.frame_ctx.active_camera)->pos;
			ImGui::Text("Camera position: %f, %f, %f", camPos.x, camPos.y, camPos.z);
			ImGui::Unindent();
			if (state.g_state.render_ui && !input.isMouseTrackingEnabled()) {
				if (ImGui::CollapsingHeader("Settings")) {
					if (ImGui::TreeNode("Player")) {
						auto* cfg = state.g_state.ecs.get_component<MovementConfig>(state.g_state.player.self);
						ImGui::SliderFloat("Player walking speed ", &cfg->walk_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Player flying speed", &cfg->fly_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Max Interaction Distance", &state.g_state.player.max_interaction_distance, 0.0f, 1000.0f);
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Camera")) {
						ImGui::SliderFloat("Mouse Sensitivity", &ctrl->sensitivity, 0.01f, 2.0f);
						ImGui::SliderFloat("FOV", &state.frame_ctx.cam->fov, 0.001f, 179.899f);
						ImGui::SliderFloat("Near Plane", &state.frame_ctx.cam->near_plane, 0.001f, 10.0f);
						ImGui::SliderFloat("Far Plane", &state.frame_ctx.cam->far_plane, 10.0f, 1000000.0f);
						ImGui::SliderFloat("Third Person Offset", &ctrl->orbit_distance, 1.0f, 200.0f);
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Miscellaneous")) {
						ImGui::SliderInt("Render distance", (int*)&state.g_state.player.render_distance, 0, 30);
						ImGui::SliderFloat("GRAVITY", &GRAVITY, -30.0f, 20.0f);
						glm::vec4 backgroundColor{};
						ImGui::SliderFloat3("BACKGROUND COLOR", &backgroundColor.r, 0.0f, 1.0f);
						GLState::set_clear_color(backgroundColor);
						static float _____width = GLState::get_line_width();
						ImGui::SliderFloat("LINE_WIDTH", &_____width, 0.001f, 9.0f);
						GLState::set_line_width(_____width);
						ImGui::Checkbox("render_terrain: ", &state.g_state.render_terrain);
						ImGui::Checkbox("render_player: ", &state.g_state.player.renderSkin);
						static bool debug = false;
						ImGui::Checkbox("debug", &debug); // Updates the value
						manager.getShader().setBool("debug", debug);
						ImGui::TreePop();
					}
				}
			}
			ImGui::End();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
#endif

		camera_temporal_system(state.g_state.ecs, state.frame_ctx);
		// other UI things...

    // TODO: Fix input handling in the manager
		input.update();       // reset this frame's state

		glBindVertexArray(0);
		glfwSwapBuffers(state.win_context->window);
#if defined(TRACY_ENABLE)
		FrameMark;
#endif
		state.frame_ctx.frame_number++;
	}	// GAME LOOP


	glDeleteVertexArrays(1, &crosshairVAO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	state.win_context.reset();

}


float getFPS(const frame_context& ctx)
{
    static int frames = 0;
    static float elapsedTime = 0.0f;
	static float lastFPS = 0.0f;
    frames++;
    elapsedTime += ctx.delta_time; // accumulate frame times

    if (elapsedTime >= 0.3f) { // update FPS every 300ms
		lastFPS = float(frames) / elapsedTime;
        frames = 0;
        elapsedTime = 0.0f;
    }

    return lastFPS; // will return 0 until first update
}

void DrawBool(const char* label, bool value)
{
	ImGui::Text("%s: ", label);
	ImGui::SameLine();
	ImGui::TextColored(value ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), value ? "TRUE" : "FALSE");
}
