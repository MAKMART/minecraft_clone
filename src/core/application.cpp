#include "core/application.hpp"
#include <GLFW/glfw3.h>
#include "core/defines.hpp"
#include "core/window_context.hpp"
#include <stb_image.h>
#include <string>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>
#endif
#include "core/raycast.hpp"
#include "game/ecs/systems/input_system.hpp"
#include "game/ecs/systems/player_state_system.hpp"
#include "game/ecs/systems/physics_system.hpp"
#include "game/ecs/systems/camera_controller_system.hpp"

Application::Application(int width, int height, std::string title) : backgroundColor(0.0f, 0.0f, 0.0f, 1.0f), context(std::make_unique<WindowContext>(width, height, title, this))
{

#if defined(DEBUG)
	std::cout << "----------------------------DEBUG MODE----------------------------\n";
#elif defined(NDEBUG)
	log::set_min_log_level(log::LogLevel::WARNING);
#else
	std::cout << "----------------------------UNKNOWN BUILD TYPE----------------------------\n";
#endif

	InputManager::get().setMouseTrackingEnabled(true);
	InputManager::get().setContext(context.get());

	glfwSetFramebufferSizeCallback(context->window, framebuffer_size_callback);

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
	                 {"\nApplication", SIZE_OF(Application)},
	                 {"\nEntity", SIZE_OF(Entity)}});

	// --- CROSSHAIR STUFF ---
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
	float centerX = width / 2.0f;
	float centerY = height / 2.0f;

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
	glCreateBuffers(1, &crosshairVBO);
	glCreateBuffers(1, &crosshairEBO);

	glNamedBufferData(crosshairVBO, Crosshairvertices.size() * sizeof(float), Crosshairvertices.data(), GL_STATIC_DRAW);

	glNamedBufferData(crosshairEBO, sizeof(CrosshairIndices), CrosshairIndices, GL_STATIC_DRAW);

	glVertexArrayVertexBuffer(crosshairVAO, 0, crosshairVBO, 0, 5 * sizeof(float));
	glVertexArrayElementBuffer(crosshairVAO, crosshairEBO);

	glVertexArrayAttribFormat(crosshairVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(crosshairVAO, 0, 0);
	glEnableVertexArrayAttrib(crosshairVAO, 0);

	glVertexArrayAttribFormat(crosshairVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
	glVertexArrayAttribBinding(crosshairVAO, 1, 0);
	glEnableVertexArrayAttrib(crosshairVAO, 1);


	// TODO: Framebuffer implementation

	log::info("Initializing Shaders...");
	playerShader     = std::make_unique<Shader>("Player", PLAYER_VERTEX_SHADER_DIRECTORY, PLAYER_FRAGMENT_SHADER_DIRECTORY);
	crossHairshader  = std::make_unique<Shader>("Crosshair", CROSSHAIR_VERTEX_SHADER_DIRECTORY, CROSSHAIR_FRAGMENT_SHADER_DIRECTORY);
	crossHairTexture = std::make_unique<Texture>(ICONS_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);
	chunkManager     = std::make_unique<ChunkManager>();
	player = std::make_unique<Player>(ecs, glm::vec3{0.0f, (float)chunkSize.y + 2.0f, 0.0f});
	ui     = std::make_unique<UI>(width, height, new Shader("UI", UI_VERTEX_SHADER_DIRECTORY, UI_FRAGMENT_SHADER_DIRECTORY), MAIN_FONT_DIRECTORY, MAIN_DOC_DIRECTORY);
	ui->SetViewportSize(width, height);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	// ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(context->window, true);
	ImGui_ImplOpenGL3_Init("#version 460");
	glLineWidth(LINE_WIDTH);
}
Application::~Application()
{
	crossHairshader.reset();
	crossHairTexture.reset();

	glDeleteVertexArrays(1, &crosshairVAO);
	glDeleteBuffers(1, &crosshairVBO);
	glDeleteBuffers(1, &crosshairEBO);

	chunkManager.reset();
	player.reset();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	context.reset();
	glfwTerminate();
}
float Application::getFPS()
{
	nbFrames++;
	// if ( deltaTime >= 0.1f ){ // If last cout was more than 1 sec ago
	float fps = float(nbFrames) / deltaTime;
	nbFrames  = 0;
	return fps;
	//}
}
void Application::processInput()
{
	// Poll keys, mouse buttons
	auto& input = InputManager::get();

	if (input.isPressed(EXIT_KEY))
		glfwSetWindowShouldClose(context->window, true);

	if (input.isPressed(FULLSCREEN_KEY))
		context->toggle_fullscreen();

	if (!input.isMouseTrackingEnabled())
		glLineWidth(LINE_WIDTH);

	// 1. Player
	if (input.isMouseTrackingEnabled()) {
		player->processMouseInput(*chunkManager);
		float scrollY = input.getScroll().y;
		if (scrollY != 0.0f)
			player->processMouseScroll(scrollY);
	}

	player->processKeyInput();

	if (input.isPressed(GLFW_KEY_H)) {
		chunkManager->getShader().reload();
		playerShader->reload();
	}
	playerShader->checkAndReloadIfModified();

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
void Application::Run()
{
	while (!glfwWindowShouldClose(context->window)) {

		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime          = currentFrame - lastFrame;
		lastFrame          = currentFrame;
		g_drawCallCount    = 0;
		UpdateFrametimeGraph(deltaTime);

		// --- Input & Event Processing ---
		glfwPollEvents();

		processInput();
		update_input(ecs);

		update_player_state(ecs, *player, deltaTime);

		Camera*           cam  = ecs.get_component<Camera>(player->getCamera());
		CameraController* ctrl = ecs.get_component<CameraController>(player->getCamera());

		// --- Chunk Generation ---
		chunkManager->generateChunks(player->getPos(), player->render_distance);

		update_physics(ecs, *chunkManager, deltaTime);
		update_camera_controller(ecs, player->getSelf());

		player->update(deltaTime);

		InputManager::get().update();
		glClearDepth(0.0f);
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
		glClear(DEPTH_TEST ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		if (renderTerrain) {
			chunkManager->getShader().checkAndReloadIfModified();
			chunkManager->renderChunks(*cam, *ecs.get_component<Transform>(player->getCamera()), glfwGetTime());
		}
#if defined(DEBUG)
		if (debugRender) {
			// Add player bounding box (with a nice greenish color)
			getDebugDrawer().addAABB(player->getAABB(), glm::vec3(0.3f, 1.0f, 0.5f));
			// Add all chunks' bounding boxes
			for (const auto& [chunkKey, chunkPtr] : chunkManager->getChunks()) {
				if (!chunkPtr)
					continue; // safety

				AABB chunkBox = chunkPtr->getAABB();

				// Color for chunk boxes, maybe a translucent blue-ish?
				glm::vec3 chunkColor(0.3f, 0.5f, 1.0f);

				getDebugDrawer().addAABB(chunkBox, chunkColor);
			}
			glm::mat4 pv = cam->projectionMatrix * cam->viewMatrix; // Render from the player's camera's prospective
			getDebugDrawer().draw(pv);
		}
#endif
		// -- Render Player -- (BEFORE UI pass)
		if (player->renderSkin) {
			playerShader->setMat4("projection", cam->projectionMatrix);
			playerShader->setMat4("view", cam->viewMatrix);
			player->render(*playerShader);
		}

		// --- UI Pass --- (now rendered BEFORE ImGui)
		if (renderUI) {
			glDisable(GL_DEPTH_TEST);
			// -- Crosshair Pass ---
			glm::mat4 orthoProj = glm::ortho(0.0f, static_cast<float>(context->getWidth()), 0.0f, static_cast<float>(context->getHeight()));
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			crossHairshader->setMat4("uProjection", orthoProj);

			crossHairTexture->Bind(2); // INFO: MAKE SURE TO BIND IT TO THE CORRECT TEXTURE BINDING!!!
			crossHairshader->use();
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
		ImGui::Text(Block::toString(player->selectedBlock));
		ImGui::SetWindowFontScale(1.0f);
		ImGui::SliderInt("Render distance", (int*)&player->render_distance, 0, 30);
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
			glm::ivec3 chunkCoords = Chunk::worldToChunk(player->getPos());
			glm::ivec3 localCoords = Chunk::worldToLocal(player->getPos());
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
			ImGui::Text("Player position: %f, %f, %f", player->getPos().x, player->getPos().y, player->getPos().z);
			ImGui::Text("Player is in chunk: %i, %i, %i", chunkCoords.x, chunkCoords.y, chunkCoords.z);
			ImGui::Text("Player local position: %d, %d, %d", localCoords.x, localCoords.y, localCoords.z);
			ImGui::Text("Player velocity: %f, %f, %f", player->getVelocity().x, player->getVelocity().y, player->getVelocity().z);
			ImGui::Text("Player MODE: %s", player->getMode());
			ImGui::Text("Player STATE: %s", player->getMovementState());
			// glm::vec3 _min = player->getAABB().min;
			// glm::vec3 _max = player->getAABB().max;
			// ImGui::Text("Player AABB { %f, %f, %f }, { %f, %f, %f }", _min.x, _min.y, _min.z, _max.x, _max.y, _max.z);
			ImGui::Text("Selected block: ");
			ImGui::SameLine();
			ImGui::SetWindowFontScale(1.2f);
			ImGui::Text("%s", Block::toString(player->selectedBlock));
			ImGui::SetWindowFontScale(1.0f);
			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::Selectable("Player flags:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			DrawBool("is OnGround", player->is_on_ground());
			DrawBool("is Damageable", player->isDamageable);
			DrawBool("is Running", player->isRunning());
			DrawBool("is Flying", player->isFlying());
			DrawBool("is Swimming", player->isSwimming());
			DrawBool("is Walking", player->isWalking());
			DrawBool("is Crouched", player->isCrouching());
			DrawBool("is third-person", ctrl->third_person);
			DrawBool("Player can place blocks", player->canPlaceBlocks);
			DrawBool("Player can break blocks", player->canBreakBlocks);
			DrawBool("renderSkin", player->renderSkin);
			ImGui::Unindent();
			ImGui::Spacing();
			ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
			ImGui::Selectable("Camera:", false, ImGuiSelectableFlags_Disabled);
			ImGui::PopStyleColor(1);
			ImGui::PopFont();
			ImGui::Indent();
			// DrawBool("is Camera interpolating", player->getCameraController().isInterpolationEnabled());
			glm::vec3& camPos = ecs.get_component<Transform>(player->getCamera())->pos;
			ImGui::Text("Camera position: %f, %f, %f", camPos.x, camPos.y, camPos.z);
			ImGui::Unindent();
			if (renderUI && !InputManager::get().isMouseTrackingEnabled()) {
				if (ImGui::CollapsingHeader("Settings")) {
					if (ImGui::TreeNode("Player")) {
						ImGui::SliderFloat("Player walking speed ", &player->walking_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Player flying speed", &player->flying_speed, 0.0f, 100.0f);
						ImGui::SliderFloat("Player running speed increment", &player->running_speed_increment, 0.0f, 100.0f);
						ImGui::SliderFloat("Max Interaction Distance", &player->max_interaction_distance, 0.0f, 100.0f);
						ImGui::SliderFloat3("armOffset", &player->getArmOffset().x, -5.0f, 5.0f);
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
						ImGui::SliderInt("Render distance", (int*)&player->render_distance, 0, 30);
						ImGui::SliderFloat("GRAVITY", &GRAVITY, -30.0f, 20.0f);
						ImGui::SliderFloat3("BACKGROUND COLOR", &backgroundColor.r, 0.0f, 1.0f);
						ImGui::SliderFloat("LINE_WIDTH", &LINE_WIDTH, 0.001f, 9.0f);
						// --- Player Model Attributes ---
						/*
						ImGui::SliderFloat3("headSize", &player->headSize.x, 0.0f, 50.0f);
						ImGui::SliderFloat3("torsoSize", &player->torsoSize.x, 0.0f, 50.0f);
						ImGui::SliderFloat3("limbSize", &player->limbSize.x, 0.0f, 50.0f);
						ImGui::SliderFloat3("headOffset", &player->headOffset.x, -10.0f, 50.0f);
						ImGui::SliderFloat3("torsoOffset", &player->torsoOffset.x, -10.0f, 50.0f);
						ImGui::SliderFloat3("rightArmOffset", &player->rightArmOffset.x, -10.0f, 50.0f);
						ImGui::SliderFloat3("leftArmOffset", &player->leftArmOffset.x, -10.0f, 50.0f);
						ImGui::SliderFloat3("rightLegOffset", &player->rightLegOffset.x, -10.0f, 50.0f);
						ImGui::SliderFloat3("leftLegOffset", &player->leftLegOffset.x, -10.0f, 50.0f);
						*/
						ImGui::Checkbox("renderTerrain", &renderTerrain);
						ImGui::Checkbox("renderPlayer", &player->renderSkin);
						static bool debug = false;
						ImGui::Checkbox("debug", &debug); // Updates the value
						chunkManager->getShader().setBool("debug", debug);
						ImGui::TreePop();
					}
				}
			}
			ImGui::End();

			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		}
#endif

		// other UI things...

		if (DEPTH_TEST) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(DEPTH_FUNC);
		}
		glPolygonMode(GL_FRONT_AND_BACK, WIREFRAME_MODE ? GL_LINE : GL_FILL);
		glfwSwapBuffers(context->window);
#if defined(TRACY_ENABLE)
		FrameMark;
#endif
	}
}

void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);

	WindowContext* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));

	if (width <= 0 || height <= 0) {
		// Don't recalculate the projection matrix, skip this frame's rendering, or log a warning
		return;
	}
	if (ctx->app) {
		ImGui::SetNextWindowPos(ImVec2(width - 300, 32), ImGuiCond_Always);
		ctx->app->ecs.get_component<Camera>(ctx->app->player->getCamera())->aspect_ratio = float(static_cast<float>(width) / height);
		ctx->app->ui->SetViewportSize(width, height);
	}
}
