#define GLM_ENABLE_EXPERIMENTAL
#include "game/ecs/systems/debug_camera_system.hpp"
#include "game/ecs/systems/camera_system.hpp"
#include "game/ecs/systems/chunk_renderer_system.hpp"
#include "game/ecs/systems/frustum_volume_system.hpp"
#include <GL/gl.h>
#if defined(TRACY_ENABLE)
#include "tracy/Tracy.hpp"
#endif
#if defined(TRACY_ENABLE)
#include <new>
void* operator new(std::size_t size)
{
	void* ptr = malloc(size);
	TracyAlloc(ptr, size);
	return ptr;
}
void operator delete(void* ptr) noexcept
{
	TracyFree(ptr);
	free(ptr);
}
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
#include "game/ecs/systems/movement_intent_system.hpp"
#include "game/ecs/systems/camera_controller_system.hpp"
#include "game/ecs/systems/input_system.hpp"
#include "game/ecs/systems/movement_physics_system.hpp"
#include "game/ecs/systems/frustum_volume_system.hpp"
#include "game/ecs/systems/temporal_camera_system.hpp"





u8  nbFrames  = 0;
f32 deltaTime = 0.0f;
f32 lastFrame = 0.0f;
std::unique_ptr<UI>            ui;
ECS                            ecs;
Player *g_player = nullptr;
framebuffer* g_fb = nullptr;
std::unique_ptr<WindowContext> context;
#if defined(DEBUG)
	b8 debugRender = false;
#endif
float getFPS();
void processInput(Player& player, Shader& playerShader, Shader& fb_shader, ChunkManager& chunkManager, Entity cam);
void DrawBool(const char* label, bool value);
//static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

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


int main()
{
	glm::vec4 backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	context = std::make_unique<WindowContext>(1920, 1080, std::string(PROJECT_NAME) + std::string(" ") + std::string(PROJECT_VERSION));

	b8 renderUI      = true;
	b8 renderTerrain = true;
	InputManager::get().setMouseTrackingEnabled(true);
	InputManager::get().setContext(context.get());


	// First thing you have to do is fix the damn stencil buffer to allow cropping in RmlUI

	// TODO:    FIX THE CAMERA CONTROLLER TO WORK WITH INTERPOLATION

#if 0
	int nExt;
	glGetIntegerv(GL_NUM_EXTENSIONS, &nExt);
	for (int i = 0; i < nExt; ++i) {
		std::cout << glGetStringi(GL_EXTENSIONS, i) << std::endl;
	}
#endif

	log::structured(log::LogLevel::INFO,
	                "SIZES",
	                {{"\nInput Manager", SIZE_OF(InputManager)},
	                 {"\nChunk Manager", SIZE_OF(ChunkManager)},
	                 {"\nShader", SIZE_OF(Shader)},
	                 {"\nTexture", SIZE_OF(Texture)},
	                 {"\nPlayer", SIZE_OF(Player)},
	                 {"\nUI", SIZE_OF(UI)},
	                 {"\nEntity", SIZE_OF(Entity)}});

	log::info("Initializing Shaders...");
	Shader playerShader("Player", PLAYER_VERTEX_SHADER_DIRECTORY, PLAYER_FRAGMENT_SHADER_DIRECTORY);
	Shader fb_shader("Framebuffer", SHADERS_DIRECTORY / "fb_vert.glsl", SHADERS_DIRECTORY / "fb_frag.glsl");
	Shader shad("Test", SHADERS_DIRECTORY / "test_vert.glsl", SHADERS_DIRECTORY / "test_frag.glsl");
	Entity debug_cam;
	debug_cam = ecs.create_entity();
	ecs.add_component(debug_cam, Camera{});
	ecs.add_component(debug_cam, Transform{});
	// Bro, you know that if you don't mark the debug_cam as "Active" it won't render a thing :)
	ecs.add_component(debug_cam, ActiveCamera{});
	ecs.add_component(debug_cam, InputComponent{});
	ecs.add_component(debug_cam, Velocity{});
	ecs.add_component(debug_cam, MovementIntent{});
	ChunkManager manager;
	Player player(ecs, glm::vec3{0.0f, (float)CHUNK_SIZE.y + 2.0f, 0.0f});
	g_player = &player;
	ui     = std::make_unique<UI>(context->getWidth(), context->getHeight(), new Shader("UI", UI_VERTEX_SHADER_DIRECTORY, UI_FRAGMENT_SHADER_DIRECTORY), MAIN_FONT_DIRECTORY, MAIN_DOC_DIRECTORY);
	ui->SetViewportSize(context->getWidth(), context->getHeight());


	framebuffer fb;
	g_fb = &fb;
	
	std::array<framebuffer_attachment_desc, 2> gbuffer_desc = {{
			{ framebuffer_attachment_type::color, GL_RGBA16F }, // albedo
			//{ framebuffer_attachment_type::color, GL_RGBA16F }, // normal
			//{ framebuffer_attachment_type::color, GL_RG16F   }, // material
			{ framebuffer_attachment_type::depth, GL_DEPTH_COMPONENT24 }
	}};

	int fb_width, fb_height;
	glfwGetFramebufferSize(context->window, &fb_width, &fb_height);
	fb.create(fb_width, fb_height, gbuffer_desc);
	fb.set_debug_name("gbuffer");

	auto framebuffer_size_callback_lambda = [](GLFWwindow* window, int width, int height) {
		glViewport(0, 0, width, height);
		if (width <= 0 || height <= 0) {
			// Don't recalculate the projection matrix, skip this frame's rendering, or log a warning
			return;
		}
		ImGui::SetNextWindowPos(ImVec2(width/* - 300*/, height), ImGuiCond_Always);
		g_fb->resize(width, height);
		ecs.get_component<Camera>(get_active_camera(ecs))->aspect_ratio = float(static_cast<float>(width) / height);
		ui->SetViewportSize(width, height);
	};

	glfwSetFramebufferSizeCallback(context->window, framebuffer_size_callback_lambda);
	// --- CROSSHAIR STUFF ---
	Shader crossHairshader("Crosshair", CROSSHAIR_VERTEX_SHADER_DIRECTORY, CROSSHAIR_FRAGMENT_SHADER_DIRECTORY);
	Texture crossHairTexture(ICONS_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);
	std::vector<float>            Crosshairvertices;
	static constexpr unsigned int CrosshairIndices[6] = {
	    0, 1, 2,
	    2, 3, 0
	};
	float crosshairSize;

	unsigned int crosshairVAO;


	std::cout << crossHairTexture.getWidth() << " x " << crossHairTexture.getHeight() << "\n";
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

	crosshairSize = 15.0f; // in pixels
	float centerX = context->getWidth() / 2.0f;
	float centerY = context->getHeight() / 2.0f;

	Crosshairvertices = {
	    centerX - crosshairSize,
	    centerY - crosshairSize,
	    1.0f,
	    uMin,
	    vMin, // Bottom-left
	    centerX + crosshairSize,
	    centerY - crosshairSize,
	    1.0f,
	    uMax,
	    vMin, // Bottom-right
	    centerX + crosshairSize,
	    centerY + crosshairSize,
	    1.0f,
	    uMax,
	    vMax, // Top-right
	    centerX - crosshairSize,
	    centerY + crosshairSize,
	    1.0f,
	    uMin,
	    vMax // Top-left
	};

	glCreateVertexArrays(1, &crosshairVAO);
	VB crosshairVBO(Crosshairvertices.data(), Crosshairvertices.size() * sizeof(float));
	IB crosshairEBO(CrosshairIndices, sizeof(CrosshairIndices));

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


    Texture Atlas(BLOCK_ATLAS_TEXTURE_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);

	GLuint dummy_vao;
	glCreateVertexArrays(1, &dummy_vao);

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





	while (!glfwWindowShouldClose(context->window)) {

		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime          = currentFrame - lastFrame;
		lastFrame          = currentFrame;
		g_drawCallCount    = 0;
		UpdateFrametimeGraph(deltaTime);

		// --- Input & Event Processing ---
		glfwPollEvents();

		Entity cammera = get_active_camera(ecs);
		processInput(player, playerShader, fb_shader, manager, cammera);

		if (InputManager::get().isPressed(GLFW_KEY_RIGHT)) {
			ecs.remove_component<ActiveCamera>(get_active_camera(ecs));
			ecs.add_component(player.getCamera(), ActiveCamera{});
		}
		if (InputManager::get().isPressed(GLFW_KEY_LEFT)) {
			ecs.remove_component<ActiveCamera>(get_active_camera(ecs));
			ecs.add_component(debug_cam, ActiveCamera{});
		}

		input_system(ecs);


		Camera*           cam  = ecs.get_component<Camera>(/*player.getCamera()*/cammera);
		CameraController* ctrl = ecs.get_component<CameraController>(player.getCamera());
		manager.generateChunks(player.getPos(), player.render_distance);

		movement_intent_system(ecs, cammera);
		movement_physics_system(ecs, manager, deltaTime);
		camera_controller_system(ecs, player.getSelf());
		debug_camera_system(ecs, deltaTime);
		camera_system(ecs);
		frustum_volume_system(ecs);

		glm::mat4 curr_view_proj = cam->projectionMatrix * cam->viewMatrix;
		glm::mat4 prev_view_proj = ecs.get_component<CameraTemporal>(player.getCamera())->prev_view_proj;
		glm::mat4 curr_inv_view_proj = glm::inverse(curr_view_proj);

		fb_shader.setMat4("curr_inv_view_proj", curr_inv_view_proj);
		fb_shader.setMat4("prev_view_proj", prev_view_proj);
		player.update(deltaTime);

		InputManager::get().update();
		glClearDepth(0.0f);
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
		fb.bind_draw();
		glClear(DEPTH_TEST ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		if (renderTerrain) {
			manager.getShader().checkAndReloadIfModified();
			chunk_renderer_system(ecs, manager, shad, deltaTime, Atlas, VAO, ecs.get_component<Camera>(cammera), *ecs.get_component<FrustumVolume>(player.getCamera()));
		}

#if defined(DEBUG)
		if (debugRender) {
			// Add player bounding box (with a nice greenish color)
			getDebugDrawer().addAABB(player.getAABB(), glm::vec3(0.3f, 1.0f, 0.5f));
			Camera* cam = ecs.get_component<Camera>(/*player.getCamera()*/cammera);
			Camera* player_cam = ecs.get_component<Camera>(player.getCamera());
			Transform* player_cam_trans = ecs.get_component<Transform>(player.getCamera());
			glm::vec3 world_forward = glm::normalize(player_cam_trans->rot * player_cam->forward);
			glm::vec3 world_up      = glm::normalize(player_cam_trans->rot * player_cam->up);
			glm::vec3 world_right   = glm::normalize(player_cam_trans->rot * player_cam->right);
			getDebugDrawer().addRay(player.getPos(), world_forward, {1.0f, 0.0f, 0.0f});
			getDebugDrawer().addRay(player.getPos(), world_up, {0.0f, 1.0f, 0.0f});
			getDebugDrawer().addRay(player.getPos(), world_right, {0.0, 0.0f, 1.0f});

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
			for (const auto& [chunkKey, chunkPtr] : manager.getChunks()) {
				if (!chunkPtr)
					continue; // safety

				AABB chunkBox = chunkPtr->getAABB();

				// Color for chunk boxes, maybe a translucent blue-ish?
				glm::vec3 chunkColor(0.3f, 0.5f, 1.0f);

				getDebugDrawer().addAABB(chunkBox, chunkColor);
			}
			getDebugDrawer().addRay(player.getPos(), glm::normalize(player.getVelocity()), {1.0f, 1.0f, 0.0f});
			getDebugDrawer().addRay(player.getPos(), glm::normalize(glm::vec3{0.0f, -GRAVITY, 0.0f}), glm::vec3(0.5f, 0.5f, 1.0f));
			// TODO: These have to be drawn with the matrices of the Entity that's currently looking at them (might be or might not be the player)
			// so using active_cam is not the best idea because everything will be displaced according to the active_camera's matrices
			glm::mat4 pv = cam->projectionMatrix * cam->viewMatrix; // Render from the active_camera's prospective
			getDebugDrawer().draw(pv);
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
		//Render everything to the framebuffer before the UI rendering that will be done on the default framebuffer, I believe!?!?
		framebuffer::bind_default_draw();
		//glClear(GL_COLOR_BUFFER_BIT);
		fb_shader.use();
		glBindTextureUnit(0, fb.color_attachment(0));
		glBindTextureUnit(1, fb.depth_attachment());
		fb_shader.setInt("color", 0);
		fb_shader.setInt("depth", 1);
		fb_shader.setFloat("time", (float)glfwGetTime());
		glBindVertexArray(dummy_vao);
		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		if (DEPTH_TEST) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(DEPTH_FUNC);
		}


		// --- UI Pass --- (now rendered BEFORE ImGui)
		if (renderUI) {
			glDisable(GL_DEPTH_TEST);
			// -- Crosshair Pass ---
			glm::mat4 orthoProj = glm::ortho(0.0f, static_cast<float>(context->getWidth()), 0.0f, static_cast<float>(context->getHeight()));
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			crossHairshader.setMat4("uProjection", orthoProj);

			crossHairTexture.Bind(2); // INFO: MAKE SURE TO BIND IT TO THE CORRECT TEXTURE BINDING!!!
			crossHairshader.use();
			glBindVertexArray(crosshairVAO);
			DrawElementsWrapper(GL_TRIANGLES, sizeof(CrosshairIndices) / sizeof(CrosshairIndices[0]), GL_UNSIGNED_INT, nullptr);
			glBindVertexArray(0);

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
			glm::ivec3 chunkCoords = Chunk::worldToChunk(player.getPos());
			glm::ivec3 localCoords = Chunk::worldToLocal(player.getPos());
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
			DrawBool("is Running", player.isRunning());
			DrawBool("is Flying", player.isFlying());
			DrawBool("is Swimming", player.isSwimming());
			DrawBool("is Walking", player.isWalking());
			DrawBool("is Crouched", player.isCrouching());
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
			glm::vec3& camPos = ecs.get_component<Transform>(/*player.getCamera()*/cammera)->pos;
			ImGui::Text("Camera position: %f, %f, %f", camPos.x, camPos.y, camPos.z);
			ImGui::Unindent();
			if (renderUI && !InputManager::get().isMouseTrackingEnabled()) {
				if (ImGui::CollapsingHeader("Settings")) {
					if (ImGui::TreeNode("Player")) {
						auto* cfg = player.getMovementConfig();
						ImGui::SliderFloat("Player walking speed ", &cfg->walk_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Player flying speed", &cfg->fly_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Max Interaction Distance", &player.max_interaction_distance, 0.0f, 100.0f);
						ImGui::TreePop();
					}
					if (ImGui::TreeNode("Camera")) {
						ImGui::SliderFloat("Mouse Sensitivity", &ctrl->sensitivity, 0.01f, 2.0f);
						ImGui::SliderFloat("FOV", &cam->fov, 0.001f, 179.899f);
						ImGui::SliderFloat("Near Plane", &cam->near_plane, 0.001f, 10.0f);
						ImGui::SliderFloat("Far Plane", &cam->far_plane, 10.0f, 1000000.0f);
						ImGui::SliderFloat("Third Person Offset", &ctrl->orbit_distance, 1.0f, 20.0f);
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
		// 3. Bind default framebuffer
		/*
		framebuffer::bind_default_draw();
		glClear(GL_COLOR_BUFFER_BIT);
		fb_shader.use();
		glBindTextureUnit(0, fb.color_attachment(0));
		glBindTextureUnit(1, fb.depth_attachment());
		fb_shader.setInt("color", 0);
		fb_shader.setInt("depth", 1);
		fb_shader.setFloat("time", (float)glfwGetTime());
		glBindVertexArray(dummy_vao);
		glDisable(GL_DEPTH_TEST);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		if (DEPTH_TEST) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(DEPTH_FUNC);
		}
		*/
		glfwSwapBuffers(context->window);
#if defined(TRACY_ENABLE)
		FrameMark;
#endif
	}


	glDeleteVertexArrays(1, &crosshairVAO);
	crosshairVBO.~VB();
	crosshairEBO.~IB();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	context.reset();
	glfwTerminate();

}


float getFPS()
{
	nbFrames++;
	// if ( deltaTime >= 0.1f ){ // If last cout was more than 1 sec ago
	float fps = float(nbFrames) / deltaTime;
	nbFrames  = 0;
	return fps;
	//}
}
void processInput(Player& player, Shader& playerShader, Shader& fb_shader, ChunkManager& chunkManager, Entity cam)
{
	// Poll keys, mouse buttons
	auto& input = InputManager::get();

	if (input.isPressed(EXIT_KEY))
		glfwSetWindowShouldClose(context->window, true);

	if (input.isPressed(FULLSCREEN_KEY))
		context->toggle_fullscreen();

	if (!input.isMouseTrackingEnabled())
		glLineWidth(LINE_WIDTH);

	float far_plane = ecs.get_component<Camera>(/*player.getCamera()*/cam)->far_plane;
	float near_plane = ecs.get_component<Camera>(/*player.getCamera()*/cam)->near_plane;

	fb_shader.setFloat("near_plane", near_plane);
	fb_shader.setFloat("far_plane", far_plane);
	static float scale = 0.01f;
	fb_shader.setFloat("scale", scale);
	if (input.isPressed(GLFW_KEY_UP)) scale+=0.01f;
	if (input.isPressed(GLFW_KEY_DOWN)) scale-=0.01f;

	static bool toggle = false;
	static bool was_pressed = false;

	bool pressed = input.isPressed(GLFW_KEY_6);

	if (pressed && !was_pressed) { // only trigger on key-down edge
		toggle = !toggle;          // flip toggle
		fb_shader.setInt("toggle", toggle ? 1 : 0);
	}

	was_pressed = pressed; // remember state for next frame





	// 1. Player
	if (input.isMouseTrackingEnabled()) {
		player.processMouseInput(chunkManager);
		float scrollY = input.getScroll().y;
		if (scrollY != 0.0f)
			player.processMouseScroll(scrollY);
	}

	player.processKeyInput();

	if (input.isPressed(GLFW_KEY_H)) {
		chunkManager.getShader().reload();
		playerShader.reload();
		fb_shader.reload();
	}
	playerShader.checkAndReloadIfModified();

	// 2. UI
	if (ui->context) {
		glm::vec2 mousePos = input.getMousePos();
		ui->context->ProcessMouseMove(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y), ui->GetKeyModifiers());

		// Mouse buttons
		for (int b = 0; b < InputManager::MAX_MOUSE_BUTTONS; ++b) {
			if (input.isMousePressed(b))
				ui->context->ProcessMouseButtonDown(b, 0);
			else if (input.isMouseReleased(b))
				ui->context->ProcessMouseButtonUp(b, 0);
		}

		// Scroll
		float scrollY = input.getScroll().y;
		if (scrollY != 0.0f)
			ui->context->ProcessMouseWheel(-scrollY, 0);

		// Keyboard
		for (int k = 0; k < GLFW_KEY_LAST; ++k) {
			if (input.isPressed(k))
				ui->context->ProcessKeyDown(ui->MapKey(k), ui->GetKeyModifiers());
			else if (input.isReleased(k))
				ui->context->ProcessKeyUp(ui->MapKey(k), ui->GetKeyModifiers());
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
}
void DrawBool(const char* label, bool value)
{
	ImGui::Text("%s: ", label);
	ImGui::SameLine();
	ImGui::TextColored(value ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), value ? "TRUE" : "FALSE");
}
