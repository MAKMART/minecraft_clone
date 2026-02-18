















/*
 *
 * TODO: IMPLEMENT THE APPSTATE AND THE MAIN MENU WITH THE NEW UI SYSTEM
 */













#include "game/ecs/components/debug_camera_controller.hpp"
#include "game/ecs/components/movement_config.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "game/ecs/systems/debug_camera_system.hpp"
#include "game/ecs/systems/camera_system.hpp"
#include "game/ecs/systems/chunk_renderer_system.hpp"
#include "game/ecs/systems/frustum_volume_system.hpp"
#if defined(TRACY_ENABLE)
#include "tracy/Tracy.hpp"
#endif
#include "core/defines.hpp"
#include "core/window_context.hpp"
#include "game/player.hpp"
#include "game/ecs/components/input.hpp"
#include "graphics/shader.hpp"
#include "graphics/renderer/framebuffer.hpp"
#include "graphics/renderer/index_buffer.hpp"
#include "ui/ui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
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
#include "game/ecs/components/render_target.hpp"
#include "game/ecs/components/debug_camera.hpp"
#include "game/ecs/components/camera_controller.hpp"
#include "game/ecs/components/active_camera.hpp"
#include "game/ecs/components/temporal_camera.hpp"
#include "game/ecs/systems/movement_intent_system.hpp"
#include "game/ecs/systems/camera_controller_system.hpp"
#include "game/ecs/systems/input_system.hpp"
#include "game/ecs/systems/movement_physics_system.hpp"
#include "game/ecs/systems/frustum_volume_system.hpp"
#include "game/ecs/systems/temporal_camera_system.hpp"
#include "graphics/renderer/framebuffer_manager.hpp"
#include "core/raycast.hpp"

std::uint64_t  nbFrames  = 0;
f32 deltaTime = 0.0f;
f32 lastFrame = 0.0f;
b8 renderUI      = true;
b8 renderTerrain = true;
std::unique_ptr<UI>            ui;
ECS                            ecs;
Player *g_player = nullptr;
FramebufferManager* g_fb_manager = nullptr;
static float crosshair_size = 0.3f;
std::unique_ptr<WindowContext> context;
#if defined(DEBUG)
	b8 debugRender = false;
#endif
float getFPS();
void DrawBool(const char* label, bool value);

Entity get_active_camera(ECS& ecs)
{
    Entity active{UINT32_MAX};
    int count = 0;

    ecs.for_each_components<Camera, ActiveCamera>(
        [&](Entity e, Camera&, ActiveCamera&) {
            active = e;
            ++count;
        }
    );

    assert(count == 1 && "There must be exactly one ActiveCamera");

   return active;
}
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
	glm::vec4 backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	context = std::make_unique<WindowContext>(1920, 1080, std::string(PROJECT_NAME) + std::string(" ") + std::string(PROJECT_VERSION));
	InputManager::get().setContext(context.get());
	InputManager::get().setMouseTrackingEnabled(true);
	auto framebuffer_size_callback_lambda = [](GLFWwindow* window, int width, int height) {
		// Don't recalculate the projection matrix, skip this frame's rendering, or log a warning
		if (width <= 0 || height <= 0) return;

		int fb_width, fb_height;
		glfwGetFramebufferSize(window, &fb_width, &fb_height);
		std::cout << "fb_width = " << fb_width << " fb_height = " << fb_height << "\n";
		glViewport(0, 0, fb_width, fb_height);
		ImGui::SetNextWindowPos(ImVec2(width/* - 300*/, height), ImGuiCond_Always);
		auto *active_cam = ecs.get_component<Camera>(get_active_camera(ecs));
		if (active_cam)
			active_cam->aspect_ratio = float(static_cast<float>(fb_width) / fb_height);
		ecs.for_each_components<RenderTarget>([&](Entity e, RenderTarget& rt) {
				if (rt.mode == extent_mode::follow_viewport) {
					rt.width  = fb_width;
					rt.height = fb_height;
					g_fb_manager->ensure(e, rt);
				}
				log::info("rt.width = {}, rt.height = {}", rt.width, rt.height);
		});
		ui->SetViewportSize(fb_width, fb_height);
	};
	glfwSetFramebufferSizeCallback(context->window, framebuffer_size_callback_lambda);

	// TODO: fix the damn stencil buffer to allow cropping in RmlUI

	// TODO:    FIX THE CAMERA CONTROLLER TO WORK WITH INTERPOLATION

#if 0
	int nExt;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nExt);
	for (int i = 0; i < nExt; ++i) {
		std::cout << glGetStringi(GL_EXTENSIONS, i) << std::endl;
	}
#endif

	log::structured(log::level::INFO,
	                "\nSIZES",{
					 {"\nInput Manager", SIZE_OF(InputManager)},
	                 {"\nChunk Manager", SIZE_OF(ChunkManager)},
	                 {"\nShader", SIZE_OF(Shader)},
	                 {"\nTexture", SIZE_OF(Texture)},
	                 {"\nPlayer", SIZE_OF(Player)},
	                 {"\nUI", SIZE_OF(UI)},
	                 {"\nEntity", SIZE_OF(Entity)}
					 });
	int fb_width, fb_height;
	glfwGetFramebufferSize(context->window, &fb_width, &fb_height);

	log::info("Initializing Shaders...");

	Shader playerShader("Player", PLAYER_VERTEX_SHADER_DIRECTORY, PLAYER_FRAGMENT_SHADER_DIRECTORY);
	Shader fb_debug("FB_DEBUG", SHADERS_DIRECTORY / "fb_vert.glsl", SHADERS_DIRECTORY / "fb_debug_frag.glsl");
	Shader fb_player("FB_PLAYER", SHADERS_DIRECTORY / "fb_vert.glsl", SHADERS_DIRECTORY / "fb_player_frag.glsl");
	Shader shad("Test", SHADERS_DIRECTORY / "test_vert.glsl", SHADERS_DIRECTORY / "test_frag.glsl");


#if defined(DEBUG)
	Entity debug_cam;
	debug_cam = ecs.create_entity();
	ecs.emplace_component<Camera>(debug_cam);
	ecs.emplace_component<Transform>(debug_cam, glm::vec3(0.0f, 10.0f, 0.0f));
	// At the start we do not need to add these componets since they will change at runtime when the player will switch to the debug camera
	//ecs.add_component(debug_cam, ActiveCamera{});
	//ecs.add_component(debug_cam, InputComponent{});
	ecs.emplace_component<Velocity>(debug_cam);
	ecs.emplace_component<MovementIntent>(debug_cam);
	ecs.emplace_component<DebugCamera>(debug_cam);
	ecs.emplace_component<DebugCameraController>(debug_cam);
	ecs.emplace_component<RenderTarget>(debug_cam, RenderTarget(context->getWidth(), context->getHeight(), {
			{ framebuffer_attachment_type::color, GL_RGBA16F }, // albedo
			//{ framebuffer_attachment_type::color, GL_RGBA16F }, // normal
			{ framebuffer_attachment_type::color, GL_RG16F   }, // material
			{ framebuffer_attachment_type::depth, GL_DEPTH_COMPONENT24 }
			}));
#endif
	ChunkManager manager;
    Texture Atlas(BLOCK_ATLAS_TEXTURE_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);
	FramebufferManager fb_manager;
	g_fb_manager = &fb_manager;
	Player player(ecs, glm::vec3{0.0f, 19.0f, 0.0f}, context->getWidth(), context->getHeight());
	g_player = &player;
	ui     = std::make_unique<UI>(context->getWidth(), context->getHeight(), MAIN_FONT_DIRECTORY);
	ui->SetViewportSize(context->getWidth(), context->getHeight());

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
	ecs.for_each_component<RenderTarget>([&](Entity e, RenderTarget& rt) {
			fb_manager.ensure(e, rt);
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
	VB crosshairVBO = VB::Immutable(verts, sizeof(verts));
	IB crosshairEBO = IB::Immutable(CrosshairIndices, std::size(CrosshairIndices)); // std::size(CrosshairIndices) == 6

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
	ImGui_ImplGlfw_InitForOpenGL(context->window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	glLineWidth(LINE_WIDTH);

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

	VB vb = VB::Immutable(vertices, sizeof(vertices));
	glVertexArrayVertexBuffer(
			VAO,
			0,                  // binding index
			vb.id(),
			0,                  // offset in buffer
			5 * sizeof(float)   // stride
			);

	manager.generate_chunks(player.getPos(), player.render_distance);
	while (!glfwWindowShouldClose(context->window)) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime          = currentFrame - lastFrame;
		lastFrame          = currentFrame;
		g_drawCallCount    = 0;
		UpdateFrametimeGraph(deltaTime);
		auto& input = InputManager::get();

		// --- Input & Event Processing ---
		glfwPollEvents();
		Entity old_camera = get_active_camera(ecs);
		if (input.isPressed(GLFW_KEY_RIGHT)) {
			ecs.remove_component<InputComponent>(old_camera);
			ecs.remove_component<ActiveCamera>(old_camera);
			ecs.add_component(player.getCamera(), ActiveCamera{});
			ecs.add_component(player.getSelf(), InputComponent{});
			// Recalculate aspect ratio cuz user might have changed to fullscreen while other cameras were active
			int fb_w, fb_h;
			glfwGetFramebufferSize(context->window, &fb_w, &fb_h);
			framebuffer_size_callback_lambda(context->window, fb_w, fb_h);
		}
		if (input.isPressed(GLFW_KEY_LEFT)) {
			ecs.remove_component<ActiveCamera>(old_camera);
			ecs.remove_component<InputComponent>(player.getSelf());
#if defined(DEBUG)
			ecs.add_component(debug_cam, ActiveCamera{});
			ecs.add_component(debug_cam, InputComponent{});
#endif
			// Recalculate aspect ratio cuz user might have changed to fullscreen while other cameras were active
			int fb_w, fb_h;
			glfwGetFramebufferSize(context->window, &fb_w, &fb_h);
			framebuffer_size_callback_lambda(context->window, fb_w, fb_h);
		}
		Entity camera = get_active_camera(ecs);

		Camera* cam = ecs.get_component<Camera>(camera);
		if (!cam)
			log::error("Couldn't find active cam for frame {}", nbFrames);

		if (input.isPressed(EXIT_KEY))
			glfwSetWindowShouldClose(context->window, true);

		if (input.isPressed(FULLSCREEN_KEY)) {
			context->toggle_fullscreen();
			int fb_w, fb_h;
			glfwGetFramebufferSize(context->window, &fb_w, &fb_h);
			framebuffer_size_callback_lambda(context->window, fb_w, fb_h);
		}

		if (!input.isMouseTrackingEnabled())
			glLineWidth(LINE_WIDTH);

		static float scale = 0.01f;
		if (camera == player.getCamera()) {
			fb_player.setFloat("near_plane", cam->near_plane);
			fb_player.setFloat("far_plane", cam->far_plane);
			fb_player.setFloat("scale", scale);
		} 
#if defined(DEBUG)
		else if (camera == debug_cam) {
			fb_debug.setFloat("near_plane", cam->near_plane);
			fb_debug.setFloat("far_plane", cam->far_plane);
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
			if (camera == player.getCamera()) {
				fb_player.setInt("toggle", toggle ? 1 : 0);
			} 
#if defined(DEBUG)
			else if (camera == debug_cam) {
				fb_debug.setInt("toggle", toggle ? 1 : 0);
			}
#endif
		}

		was_pressed = pressed; // remember state for next frame

		// 1. Player
		if (input.isMouseTrackingEnabled()) {
			if (input.isMousePressed(ATTACK_BUTTON) && player.canBreakBlocks && camera == player.getCamera()) {
				player.breakBlock(manager);
			}
			if (input.isMousePressed(DEFENSE_BUTTON) && player.canPlaceBlocks && camera == player.getCamera()) {
				player.placeBlock(manager);
			}
			if (camera == debug_cam) {
				Transform* trans = ecs.get_component<Transform>(camera);
				if (!trans)
					log::error("NO TRANSFORM FOR ENTITY: {}", camera.id);
				glm::vec4 clip = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

				glm::vec4 view_space = glm::inverse(cam->projectionMatrix) * clip;
				view_space.z = -1.0f; // forward direction
				view_space.w = 0.0f;  // this is a direction, not a position

				glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(cam->viewMatrix) * view_space));
				glm::vec3 ray_origin = trans->pos; // start at camera position
				if (input.isMouseHeld(ATTACK_BUTTON)) {
					std::optional<glm::ivec3> hitBlock = raycast::voxel(manager, ray_origin, ray_dir, player.max_interaction_distance);
					if (hitBlock.has_value()) {
						glm::ivec3 blockPos = hitBlock.value();
						manager.updateBlock(blockPos, Block::blocks::AIR);
					}
					// log::info("Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);
				}
				if (input.isMouseHeld(DEFENSE_BUTTON)) {
					std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycast::voxel_normals(manager, ray_origin, ray_dir, player.max_interaction_distance);
					if (hitResult.has_value()) {
						glm::ivec3 hitBlockPos = hitResult->first;
						glm::ivec3 normal      = hitResult->second;
						glm::ivec3 placePos = hitBlockPos + (-normal);
						glm::ivec3 localBlockPos = Chunk::world_to_local(placePos);

						if (manager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z) != Block::blocks::AIR) {
							log::error("❌ Target block is NOT air! It's type: {}", Block::toString(manager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z)));
						}

						//log::info("Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);
						/*
						int radius = 2;
						for(int i = -radius; i < radius; i++) {
							for(int j = -radius; j < radius; j++) {
								for(int k = -radius; k < radius; k++)
									manager.updateBlock(placePos + glm::ivec3(i, j, k), static_cast<Block::blocks>(player.selectedBlock));
							}
						}
						*/
						manager.updateBlock(placePos, static_cast<Block::blocks>(player.selectedBlock));
					}
				}
			
			}


			float scrollY = input.getScroll().y;
			if (scrollY != 0.0f) {
				CameraController* ctrl = ecs.get_component<CameraController>(camera);
				float scroll_speed_multiplier = 1.0f;
				if (!ctrl->third_person) {
					player.selectedBlock += static_cast<int>(scrollY * scroll_speed_multiplier);
					if (player.selectedBlock < 1)
						player.selectedBlock = 1;
					if (player.selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS))
						player.selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
				}

				// To change FOV
				// camCtrl.processMouseScroll(scrollY);


			}
		}

		PlayerMode* mode = ecs.get_component<PlayerMode>(player.getSelf());
		PlayerState* state = ecs.get_component<PlayerState>(player.getSelf());
		if (input.isPressed(CAMERA_SWITCH_KEY) && ecs.has_component<CameraController>(camera)) {
			ecs.get_component<CameraController>(camera)->third_person = !ecs.get_component<CameraController>(camera)->third_person;
		}

		if (input.isPressed(SURVIVAL_MODE_KEY)) {
			state->current = PlayerMovementState::Walking;
		}

		if (input.isPressed(CREATIVE_MODE_KEY)) {
			state->current = PlayerMovementState::Flying;
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

		if (input.isPressed(GLFW_KEY_ENTER) && camera == debug_cam) {
			ecs.get_component<Transform>(player.getSelf())->pos = ecs.get_component<Transform>(debug_cam)->pos;
			ecs.get_component<Velocity>(player.getSelf())->value = ecs.get_component<Velocity>(debug_cam)->value;
		
		}
		
		

		// 2. UI
		if (ui->get_context()) {
			glm::vec2 mousePos = input.getMousePos();
			ui->get_context()->ProcessMouseMove(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y), ui->GetKeyModifiers());

			// Mouse buttons
			for (int b = 0; b < InputManager::MAX_MOUSE_BUTTONS; ++b) {
				if (input.isMousePressed(b))
					ui->get_context()->ProcessMouseButtonDown(b, 0);
				else if (input.isMouseReleased(b))
					ui->get_context()->ProcessMouseButtonUp(b, 0);
			}

			// Scroll
			float scrollY = input.getScroll().y;
			if (scrollY != 0.0f)
				ui->get_context()->ProcessMouseWheel(-scrollY, 0);

			// Keyboard
			for (int k = 0; k < GLFW_KEY_LAST; ++k) {
				if (input.isPressed(k))
					ui->get_context()->ProcessKeyDown(ui->MapKey(k), ui->GetKeyModifiers());
				else if (input.isReleased(k))
					ui->get_context()->ProcessKeyUp(ui->MapKey(k), ui->GetKeyModifiers());
			}
		}

#if defined(DEBUG)
		if (input.isPressed(GLFW_KEY_8)) {
			debugRender = !debugRender;
		}
#endif

		if (input.isPressed(MENU_KEY)) {
#if defined(DEBUG)
			Rml::Debugger::SetVisible(input.isMouseTrackingEnabled());
#endif
			input.setMouseTrackingEnabled(!input.isMouseTrackingEnabled());
		}

#if defined(DEBUG)
		if (input.isPressed(WIREFRAME_KEY)) {
			glPolygonMode(GL_FRONT_AND_BACK, WIREFRAME_MODE ? GL_LINE : GL_FILL);
			WIREFRAME_MODE = !WIREFRAME_MODE;
		}
#endif



		input_system(ecs);

		CameraController* ctrl = ecs.get_component<CameraController>(player.getCamera());
		manager.generate_chunks(player.getPos(), player.render_distance);

		movement_intent_system(ecs, camera);
		movement_physics_system(ecs, manager, deltaTime);
		camera_controller_system(ecs, player.getSelf());
#if defined(DEBUG)
		debug_camera_system(ecs, deltaTime);
#endif
		camera_system(ecs);
		frustum_volume_system(ecs);


		player.update(deltaTime);

		glClearDepth(0.0f);
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
		glClear(DEPTH_TEST ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);




#if defined(DEBUG)
		if (debugRender) {
			getDebugDrawer().addAABB(player.getAABB(), glm::vec3(0.3f, 1.0f, 0.5f));
			Camera* player_cam = ecs.get_component<Camera>(player.getCamera());
			Transform* player_cam_trans = ecs.get_component<Transform>(player.getCamera());
			glm::vec3 world_forward = glm::normalize(player_cam_trans->rot * player_cam->forward);
			glm::vec3 world_up      = glm::normalize(player_cam_trans->rot * player_cam->up);
			glm::vec3 world_right   = glm::normalize(player_cam_trans->rot * player_cam->right);
			getDebugDrawer().addRay(player.getPos(), world_forward, {1.0f, 0.0f, 0.0f});
			getDebugDrawer().addRay(player.getPos(), world_up, {0.0f, 1.0f, 0.0f});
			getDebugDrawer().addRay(player.getPos(), world_right, {0.0, 0.0f, 1.0f});

			ecs.for_each_components<Camera, Transform>([](Entity e, Camera, Transform& trans){
					if (!ecs.has_component<ActiveCamera>(e))
						getDebugDrawer().addAABB(AABB::fromCenterSize(trans.pos, {0.5f, 0.8f, 0.5f}), glm::vec3(1.0f, 0.0f, 1.0f));
					});

			float ndc_z = -1.0f; // near plane
			glm::vec4 clip = glm::vec4(0.0f, 0.0f, ndc_z, 1.0f);

			glm::vec4 view_space = glm::inverse(player_cam->projectionMatrix) * clip;
			view_space.z = -1.0f; // forward direction
			view_space.w = 0.0f;  // this is a direction, not a position

			glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(player_cam->viewMatrix) * view_space));
			glm::vec3 cam_pos   = player_cam_trans->pos + glm::vec3(0.1, 0.0, 0.1f);
			getDebugDrawer().addRay(cam_pos, ray_dir, {1.0f, 0.0f, 0.0f});
			glm::vec3 cam_dir   = glm::normalize(player_cam_trans->rot * cam->forward);
			getDebugDrawer().addRay(cam_pos, cam_dir, {1.0f, 0.0f, 0.0f});


			// Add all chunks' bounding boxes
			for (const auto& [chunkKey, chunkPtr] : manager.get_all()) {
				if (!chunkPtr)
					continue; // safety

				AABB chunkBox = chunkPtr->getAABB();

				// Color for chunk boxes, maybe a translucent blue-ish?
				glm::vec3 chunkColor(0.3f, 0.5f, 1.0f);

				getDebugDrawer().addAABB(chunkBox, chunkColor);
			}
			getDebugDrawer().addRay(player.getPos(), glm::normalize(player.getVelocity()), {1.0f, 1.0f, 0.0f});
			getDebugDrawer().addRay(player.getPos(), glm::normalize(glm::vec3{0.0f, -GRAVITY, 0.0f}), glm::vec3(0.5f, 0.5f, 1.0f));
		}
#endif
		// -- Render Player -- (BEFORE UI pass)
		// TODO: Actually fix and implement this shit
		/*
		if (player.renderSkin) {
			playerShader.setMat4("projection", cam->projectionMatrix);
			playerShader.setMat4("view", cam->viewMatrix);
			player.render(playerShader);
		}
		*/


		if (renderTerrain) {
			//manager.getShader().checkAndReloadIfModified();
			manager.getShader().setMat4("projection", cam->projectionMatrix);
			manager.getShader().setMat4("view", cam->viewMatrix);
			manager.getShader().setFloat("time", (float)glfwGetTime());
			Atlas.Bind(0);
			manager.getShader().use();
			// Whatever VAO'll work
			glBindVertexArray(VAO);
			chunk_renderer_system(ecs, manager, cam, *ecs.get_component<FrustumVolume>(player.getCamera()), fb_manager);
			glBindVertexArray(0);
			Atlas.Unbind(0);
		}

		// Debug render
#if defined(DEBUG)
		if (debugRender) {
			ecs.for_each_components<Camera, Transform, RenderTarget, ActiveCamera>([](Entity e, Camera& cam, Transform, RenderTarget, ActiveCamera){
					glm::mat4 pv = cam.projectionMatrix * cam.viewMatrix;
					getDebugDrawer().draw(pv);
					});
		}
#endif

		glm::mat4 temp_model = glm::mat4(1.0f);
		temp_model = glm::translate(temp_model, glm::vec3(-0.5f, 9.5f, 0.0f));
		temp_model = glm::scale(temp_model, glm::vec3(0.5f, 0.5f, 0.5f));
		shad.setMat4("projection", cam->projectionMatrix);
		shad.setMat4("model", temp_model);
		shad.setMat4("view", cam->viewMatrix);

		Atlas.Bind(0);
		shad.use();
		glBindVertexArray(VAO);
		DrawArraysWrapper(GL_TRIANGLES, 0, 3);
		Atlas.Unbind(0);



		framebuffer::bind_default_draw();
		glViewport(0, 0, context->getWidth(), context->getHeight());
		glDisable(GL_DEPTH_TEST);
		auto& cur_fb = fb_manager.get(camera);
		glViewport(0, 0, cur_fb.width(), cur_fb.height());
		glBindTextureUnit(0, cur_fb.color_attachment(0));
		glBindTextureUnit(1, cur_fb.depth_attachment());
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cube_id);

		glm::mat4 curr_view_proj = cam->projectionMatrix * cam->viewMatrix;
		glm::mat4 curr_inv_view_proj = glm::inverse(curr_view_proj);

		if (camera == player.getCamera()) {
			glm::mat4 prev_view_proj = ecs.get_component<CameraTemporal>(player.getCamera())->prev_view_proj;
			fb_player.setMat4("curr_inv_view_proj", curr_inv_view_proj);
			fb_player.setMat4("prev_view_proj", prev_view_proj);
			fb_player.setMat4("invView", glm::inverse(cam->viewMatrix));
			fb_player.setMat4("invProj", glm::inverse(cam->projectionMatrix));
			fb_player.setInt("color", 0);
			fb_player.setInt("depth", 1);
			fb_player.setInt("skybox", 2);
			fb_player.use();
		} 
#if defined(DEBUG)
		else if (camera == debug_cam) {
			fb_debug.setInt("color", 0);
			fb_debug.setInt("depth", 1);
			fb_debug.use();
		}
#endif

		// NOTE: Make sure to have a VAO bound when making this draw call!!
		DrawArraysWrapper(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		// --- UI Pass --- (now rendered BEFORE ImGui)
		if (renderUI/* && camera == player.getCamera()*/) {
			glDisable(GL_DEPTH_TEST);
			// -- Crosshair Pass ---
				glm::mat4 orthoProj = glm::ortho(0.0f, static_cast<float>(cur_fb.width()), 0.0f, static_cast<float>(cur_fb.height()));
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			crossHairshader.setMat4("uProjection", orthoProj);
			crossHairshader.setVec2("uCenter", glm::vec2(cur_fb.width() * 0.5f, cur_fb.height() * 0.5f));
			crossHairshader.setFloat("uSize", crosshair_size);

			crossHairTexture.Bind(2); // INFO: MAKE SURE TO BIND IT TO THE CORRECT TEXTURE BINDING!!!
			crossHairshader.use();
			glBindVertexArray(crosshairVAO);
			DrawElementsWrapper(GL_TRIANGLES, sizeof(CrosshairIndices) / sizeof(CrosshairIndices[0]), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

			if (camera == player.getCamera())
				ui->render();
			
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
		ImGui::Text("FPS: %f", getFPS());
		ImGui::Text("Selected block: ");
		ImGui::SameLine();
		ImGui::SetWindowFontScale(1.2f);
		ImGui::Text(Block::toString(player.selectedBlock));
		ImGui::SetWindowFontScale(1.0f);
		ImGui::SliderInt("Render distance", (int*)&player.render_distance, 0, 30);
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

#if defined(DEBUG)
		if (renderUI) {
			// --- ImGui Debug UI Pass ---
			glDisable(GL_DEPTH_TEST);
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			glm::ivec3 chunkCoords = Chunk::world_to_chunk(player.getPos());
			glm::ivec3 localCoords = Chunk::world_to_local(player.getPos());
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
			ImGui::Text("FPS: %f", getFPS());
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
			ImGui::Text("Player position: %f, %f, %f", player.getPos().x, player.getPos().y, player.getPos().z);
			ImGui::Text("Player is in chunk: %i, %i, %i", chunkCoords.x, chunkCoords.y, chunkCoords.z);
			ImGui::Text("Player local position: %d, %d, %d", localCoords.x, localCoords.y, localCoords.z);
			ImGui::Text("Player velocity: %f, %f, %f", player.getVelocity().x, player.getVelocity().y, player.getVelocity().z);
			ImGui::Text("Player MODE: %s", player.getMode());
			ImGui::Text("Player STATE: %s", player.getMovementState());
			// glm::vec3 _min = player.getAABB().min;
			// glm::vec3 _max = player.getAABB().max;
			// ImGui::Text("Player AABB { %f, %f, %f }, { %f, %f, %f }", _min.x, _min.y, _min.z, _max.x, _max.y, _max.z);
			ImGui::Text("Selected block: ");
			ImGui::SameLine();
			ImGui::SetWindowFontScale(1.2f);
			ImGui::Text("%s", Block::toString(player.selectedBlock));
			ImGui::SetWindowFontScale(1.0f);
			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::Selectable("Player flags:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			DrawBool("is OnGround", player.is_on_ground());
			DrawBool("is Damageable", player.isDamageable);
			DrawBool("is Running", state->current == PlayerMovementState::Running);
			DrawBool("is Flying", state->current == PlayerMovementState::Flying);
			DrawBool("is Swimming", state->current == PlayerMovementState::Swimming);
			DrawBool("is Walking", state->current == PlayerMovementState::Walking);
			DrawBool("is Crouched", state->current == PlayerMovementState::Crouching);
			DrawBool("is third-person", ctrl->third_person);
			DrawBool("Player can place blocks", player.canPlaceBlocks);
			DrawBool("Player can break blocks", player.canBreakBlocks);
			DrawBool("renderSkin", player.renderSkin);
			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
			ImGui::Selectable("Camera:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			// DrawBool("is Camera interpolating", player.getCameraController().isInterpolationEnabled());
			glm::vec3& camPos = ecs.get_component<Transform>(camera)->pos;
			ImGui::Text("Camera position: %f, %f, %f", camPos.x, camPos.y, camPos.z);
			ImGui::Unindent();
			if (renderUI && !input.isMouseTrackingEnabled()) {
				if (ImGui::CollapsingHeader("Settings")) {
					if (ImGui::TreeNode("Player")) {
						auto* cfg = ecs.get_component<MovementConfig>(player.getSelf());
						ImGui::SliderFloat("Player walking speed ", &cfg->walk_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Player flying speed", &cfg->fly_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Max Interaction Distance", &player.max_interaction_distance, 0.0f, 1000.0f);
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Camera")) {
						ImGui::SliderFloat("Mouse Sensitivity", &ctrl->sensitivity, 0.01f, 2.0f);
						ImGui::SliderFloat("FOV", &cam->fov, 0.001f, 179.899f);
						ImGui::SliderFloat("Near Plane", &cam->near_plane, 0.001f, 10.0f);
						ImGui::SliderFloat("Far Plane", &cam->far_plane, 10.0f, 1000000.0f);
						ImGui::SliderFloat("Third Person Offset", &ctrl->orbit_distance, 1.0f, 200.0f);
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Miscellaneous")) {
						ImGui::SliderInt("Render distance", (int*)&player.render_distance, 0, 30);
						ImGui::SliderFloat("GRAVITY", &GRAVITY, -30.0f, 20.0f);
						ImGui::SliderFloat3("BACKGROUND COLOR", &backgroundColor.r, 0.0f, 1.0f);
						ImGui::SliderFloat("LINE_WIDTH", &LINE_WIDTH, 0.001f, 9.0f);
						ImGui::Checkbox("renderTerrain", &renderTerrain);
						ImGui::Checkbox("renderPlayer", &player.renderSkin);
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

		camera_temporal_system(ecs);
		// other UI things...

		if (DEPTH_TEST) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(DEPTH_FUNC);
		}
		glPolygonMode(GL_FRONT_AND_BACK, WIREFRAME_MODE ? GL_LINE : GL_FILL);

		glBindVertexArray(0);
		input.update();
		glfwSwapBuffers(context->window);
#if defined(TRACY_ENABLE)
		FrameMark;
#endif
		nbFrames++;
	}


	glDeleteVertexArrays(1, &crosshairVAO);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	context.reset();
	glfwTerminate();

}


float getFPS()
{
    static int frames = 0;
    static float elapsedTime = 0.0f;
	static float lastFPS = 0.0f;
    frames++;
    elapsedTime += deltaTime; // accumulate frame times

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
