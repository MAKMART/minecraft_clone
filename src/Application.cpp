#include "Application.h"
#include "Debugger/Debugger.h"
#include "GLFW/glfw3.h"
#include "defines.h"
#include "imgui.h"
#include <exception>
#include <memory>
#include <stb_image.h>
#include <stdexcept>

Application::Application(int width, int height)
    : title(std::string(PROJECT_NAME) + " " + PROJECT_VERSION),
    width(width), height(height), backgroundColor(0.11f, 0.67f, 0.7f, 1.0f),
    nbFrames(0), deltaTime(0.0f), lastFrame(0.0f), 
    mouseClickEnabled(true), frametimes(frametime_max, 0.0f) {

#ifdef DEBUG
    std::cout << "----------------------------DEBUG MODE----------------------------\n";
#else
#ifdef NDEBUG
    std::cout << "----------------------------RELEASE MODE----------------------------\n";
#else
    std::cout << "----------------------------UNKNOWN BUILD TYPE----------------------------\n";
#endif
#endif


    initWindow();
    // Set the Application pointer for callbacks.
    glfwSetWindowUserPointer(window, this);


    // --- CROSSHAIR STUFF ---
    // Define texture and cell size
    const float textureWidth = 512.0f;
    const float textureHeight = 512.0f;
    const float cellWidth = 15.0f;
    const float cellHeight = 15.0f;

    // Define row and column (0-based index, top-left starts at row=0, col=0)
    const int row = 0;    // First row (from the top)
    const int col = 0;    // First column

    // Calculate texture coordinates
    float uMin = (col * cellWidth) / textureWidth;
    float vMin = 1.0f - ((row + 1) * cellHeight) / textureHeight; // Flip Y to start from top
    float uMax = ((col + 1) * cellWidth) / textureWidth;
    float vMax = 1.0f - (row * cellHeight) / textureHeight;

    crosshairSize = 15.0f; // in pixels
    float centerX = width / 2.0f;
    float centerY = height / 2.0f;

    // Define vertices in screen coordinates (x, y) and texture coords
    Crosshairvertices = {
	// Positions:         					  // Texture Coords:
	centerX - crosshairSize, centerY - crosshairSize, 1.0f,   uMin, vMin, // Bottom-left
	centerX + crosshairSize, centerY - crosshairSize, 1.0f,   uMax, vMin, // Bottom-right
	centerX + crosshairSize, centerY + crosshairSize, 1.0f,   uMax, vMax, // Top-right
	centerX - crosshairSize, centerY + crosshairSize, 1.0f,   uMin, vMax  // Top-left
    };


    // Generate VAO and Buffers with DSA
    glCreateVertexArrays(1, &crosshairVAO);
    glCreateBuffers(1, &crosshairVBO);
    glCreateBuffers(1, &crosshairEBO);

    // Set the data for the Vertex Array Object (VAO) and Buffers directly

    // Buffer data for crosshair VBO
    glNamedBufferData(crosshairVBO, Crosshairvertices.size() * sizeof(float), Crosshairvertices.data(), GL_STATIC_DRAW);

    // Buffer data for crosshair EBO
    glNamedBufferData(crosshairEBO, sizeof(CrosshairIndices), CrosshairIndices, GL_STATIC_DRAW);

    // Set up vertex attributes for the VAO using DSA
    glVertexArrayVertexBuffer(crosshairVAO, 0, crosshairVBO, 0, 5 * sizeof(float)); // Specify the vertex buffer for the VAO
    glVertexArrayElementBuffer(crosshairVAO, crosshairEBO); // Specify the index buffer for the VAO

    // Position attribute
    glVertexArrayAttribFormat(crosshairVAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(crosshairVAO, 0, 0);
    glEnableVertexArrayAttrib(crosshairVAO, 0);

    // Texture coordinate attribute
    glVertexArrayAttribFormat(crosshairVAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(crosshairVAO, 1, 0);
    glEnableVertexArrayAttrib(crosshairVAO, 1);


    // Use smart pointers instead of raw pointers.
    std::cout << "Initializing Input Manager...\n";
    input = std::make_unique<InputManager>(window);
    std::cout << "Initializing Shaders...\n";
    try {
    playerShader = std::make_unique<Shader>(PLAYER_VERTEX_SHADER_DIRECTORY, PLAYER_FRAGMENT_SHADER_DIRECTORY);
    } catch (const std::exception& e) {
	std::cerr << "Failed to create Shader!!\n";
    }
    crossHairshader = std::make_unique<Shader>(CROSSHAIR_VERTEX_SHADER_DIRECTORY, CROSSHAIR_FRAGMENT_SHADER_DIRECTORY);
    std::cout << "Initializing Textures...\n";
    crossHairTexture = std::make_unique<Texture>(ICONS_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST);
    std::cout << "Initializing Player...\n";
    player = std::make_unique<Player>(glm::vec3(0.0f, (float)(chunkSize.y) - 0.1f, 0.0f), chunkSize, window);
    std::cout << "Initializing Chunk Manager...\n";
    try {
    chunkManager = std::make_unique<ChunkManager>(chunkSize, player->render_distance);
    } catch (const std::exception& e) {
	std::cerr << "Error: " << e.what() << std::endl;
    }
    ui = new UI(width, height, new Shader(UI_VERTEX_SHADER_DIRECTORY, UI_FRAGMENT_SHADER_DIRECTORY), MAIN_FONT_DIRECTORY, MAIN_DOC_DIRECTORY);
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGuiIO& io = ImGui::GetIO(); // Avoid unused variable warning if not using io directly
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
   ImGui_ImplOpenGL3_Init("#version 460");
}
Application::~Application(void) {
    // Clean up ImGui and GLFW and UI
    delete ui;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
void Application::initWindow(void) {
    if (!glfwInit()) {
	throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
#endif
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);		// !!!!!!!!!!!!!! IMPORTANT !!!!!!!!!!!!!!!!!!!!!!!!
#ifdef DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

    glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, MSAA_STRENGHT);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, DEPTH_BITS);
    glfwWindowHint(GLFW_STENCIL_BITS, STENCIL_BITS);
    glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    // Get primary monitor and its video mode
    //if(isFullscreen) {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    //}

    window = (isFullscreen)
	? glfwCreateWindow(mode->width, mode->height, title.c_str(), monitor, nullptr)
	: glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window) {
	glfwTerminate();
	throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(window);

    // Set GLFW callbacks
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    if (glfwRawMouseMotionSupported()) {
	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    } else {
	std::cerr << "Raw Mouse Motion not supported!\n";
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    GLuint err = glewInit();
    if (err != GLEW_OK) {
	const GLubyte* errorStr = glewGetErrorString(err);
	std::string errorMessage = errorStr ? std::string(reinterpret_cast<const char*>(errorStr)) : "Unknown GLEW error";
	throw std::runtime_error("Failed to set window icon, OpenGL error: " + errorMessage);
	return;
    }

    // --- DEBUG SETUP ---
#ifdef DEBUG
    if (glfwExtensionSupported("GL_KHR_debug")) {
	std::cout << "Debug Output is supported!" << std::endl;
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(Application::MessageCallback, 0);
	GLenum err2 = glGetError();
	if (err2 != GL_NO_ERROR) {
	    std::cerr << "Failed to set debug callback, OpenGL error: " << glewGetErrorString(err2) << std::endl;
	}
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    } else {
	std::cerr << "Debug Output is not supported by this OpenGL context!" << std::endl;
    }
#endif

    // Set window icon
    int icon_width, icon_height, icon_channels;
    unsigned char* icon_data = stbi_load(WINDOW_ICON_DIRECTORY.string().c_str(), &icon_width, &icon_height, &icon_channels, 4);

    if (icon_data) {
	std::cout << "Loading window icon " << icon_width << "x" << icon_height << " with " << icon_channels << " channels...\n";

	GLFWimage image;
	image.width = icon_width;
	image.height = icon_height;
	image.pixels = icon_data;

	glfwSetWindowIcon(window, 1, &image);
	stbi_image_free(icon_data);
    } else {
	throw std::runtime_error("Failed to load icon: " + std::string(stbi_failure_reason()));
    }

    GLenum err3 = glGetError();
    if (err3 != GL_NO_ERROR) {
	std::string errorMessage;
	switch (err3) {
	    case GL_INVALID_VALUE: errorMessage = "Invalid value"; break;
	    case GL_INVALID_OPERATION: errorMessage = "Invalid operation"; break;
	    default: errorMessage = "Unknown OpenGL error (" + std::to_string(err3) + ")";
	}
	throw std::runtime_error("Failed to set window icon, OpenGL error: " + errorMessage);	
    }


    glViewport(0, 0, width, height);
    if(!V_SYNC)
	glfwSwapInterval(0); // Disables V-Sync
    else
	glfwSwapInterval(1);

    // OpenGL state setup
    if(FACE_CULLING) {
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
    }
    if(DEPTH_TEST) {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL); // Closer fragments overwrite farther ones
    }
    if(BLENDING) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    if(MSAA)		glEnable(GL_MULTISAMPLE);

    aspectRatio = (float)width / (float)height;
}
float Application::getFPS(float delta)
{
     nbFrames++;
     //if ( delta >= 0.1f ){ // If last cout was more than 1 sec ago
         float fps = float(nbFrames) / delta;
         nbFrames = 0;
	 return fps;
     //}
}
void Application::processInput() {
    input->update();
    if (input->isPressed(EXIT_KEY))
        glfwSetWindowShouldClose(window, true);

    handleFullscreenToggle(window);

    // --- Process Mouse Buttons ---
    if(mouseClickEnabled)
    {
	if (input->isMousePressed(ATTACK_BUTTON)) {
	    player->processMouseInput(Player::ACTION::BREAK_BLOCK, *chunkManager);
	}
	if (input->isMousePressed(DEFENSE_BUTTON)) {
	    player->processMouseInput(Player::ACTION::PLACE_BLOCK, *chunkManager);
	}
    }

    // Process mode switches (note: consider safety for mode/state assignments)
    if (input->isPressed(SURVIVAL_MODE_KEY)) {
        // Switch to Survival mode
        player->changeMode(std::make_unique<SurvivalMode>());
        player->changeState(std::make_unique<WalkingState>()); // Default state in Survival is Walking
    }
    if (input->isPressed(CREATIVE_MODE_KEY)) {
        // Switch to Creative mode
        player->changeMode(std::make_unique<CreativeMode>());
    }
    if (input->isPressed(SPECTATOR_MODE_KEY)) {
        // Switch to Spectator mode
        player->changeMode(std::make_unique<SpectatorMode>());
    }
    if (input->isPressed(SPRINT_KEY) && !player->isFlying && !player->isSwimming && player->isOnGround)
	player->changeState(std::make_unique<RunningState>());
    if (input->isReleased(SPRINT_KEY) && !player->isFlying && !player->isSwimming)
	player->changeState(std::make_unique<WalkingState>());


    // Toggle third-person/first-person camera
    if (input->isPressed(CAMERA_SWITCH_KEY)) {
        player->toggleCameraMode(); }

    if (input->isPressed(MENU_KEY))
    {
	player->_camera->setMouseTracking(FREE_CURSOR);
	Rml::Debugger::SetVisible(!FREE_CURSOR);
	glfwSetInputMode(window, GLFW_CURSOR, FREE_CURSOR ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	mouseClickEnabled = !mouseClickEnabled;
	FREE_CURSOR = !FREE_CURSOR;
    }
    // Toggle wireframe mode with cooldown
#ifdef DEBUG
    if (input->isPressed(WIREFRAME_KEY))
    {
	glPolygonMode(GL_FRONT_AND_BACK, WIREFRAME_MODE ? GL_LINE : GL_FILL);
        WIREFRAME_MODE = !WIREFRAME_MODE;
    }
#endif
}

void Application::handleFullscreenToggle(GLFWwindow* window) {
    if (input->isPressed(FULLSCREEN_KEY)) {
        if (isFullscreen) {
            glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
            isFullscreen = false;
        } else {
            const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
            glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
            glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
            glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, mode->refreshRate);
            isFullscreen = true;
        }
    }
}
void Application::Run(void) {
    if(!crossHairshader)
    {
	throw std::runtime_error("Crosshair shader not initialized!");
	return;
    }
    if(!playerShader)
    {
	throw std::runtime_error("Player shader not initialized!");
	return;
    }

    while(!glfwWindowShouldClose(window) && window) {
	// --- Time Management ---
	float currentFrame = static_cast<float>(glfwGetTime());
	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;
	g_drawCallCount = 0;
	UpdateFrametimeGraph(deltaTime);

	// --- Input & Event Processing ---
	glfwPollEvents();
	if(!mouseClickEnabled)	glLineWidth(LINE_WIDTH);
	processInput();

	// --- Clear Screen ---
	glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
	glClear(DEPTH_TEST ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT : GL_COLOR_BUFFER_BIT);

	// --- Player Update && Third-Person Rendering ---
	playerShader->setMat4("projection", player->_camera->GetProjectionMatrix());
	playerShader->setMat4("view", player->_camera->GetViewMatrix());
	player->update(deltaTime, *chunkManager);

	// --- Render World ---
	if(renderTerrain) {
	  chunkManager->renderChunks(player->getPos(), player->render_distance, *player->_camera);
	  //chunkManager->chunkShader->setFloat("time", glfwGetTime());	// yk maybe don't do it here 
									// would be better if you do it in the ChunkManger class directly
									// TODO: FIX it
	}
	// -- Render Player -- (BEFORE UI pass)
	if(player->renderSkin) {
	    playerShader->use();
	    player->render(playerShader->getProgramID());
	}

	// --- UI Pass --- (now rendered BEFORE ImGui)
	if(renderUI) {
	    glDisable(GL_DEPTH_TEST);
	    // -- Crosshair Pass ---
	    glm::mat4 orthoProj = glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height));
	    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	    crossHairshader->setMat4("uProjection", orthoProj);

	    crossHairTexture->Bind(1);	//INFO: MAKE SURE TO BIND IT TO THE CORRECT TEXTURE BINDING!!!
	    crossHairshader->use();
	    glBindVertexArray(crosshairVAO);
	    DrawElementsWrapper(GL_TRIANGLES, sizeof(CrosshairIndices) / sizeof(CrosshairIndices[0]), GL_UNSIGNED_INT, nullptr);
	    glBindVertexArray(0);
	    // --- ---

	    //render the ui before IMGUI
	    
	    ui->render();
	    //other UI stuff...
	}
#ifdef NDEBUG // TEMP
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("INFO", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);
	RenderFrametimeGraph();
	ImGui::SliderInt("Render distance", (int*)&player->render_distance, 0, 30);
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

#ifdef DEBUG
	if(renderUI) {
	    // --- ImGui Debug UI Pass ---
	    glDisable(GL_DEPTH_TEST);
	    ImGui_ImplOpenGL3_NewFrame();
	    ImGui_ImplGlfw_NewFrame();
	    ImGui::NewFrame();
	    glm::ivec3 chunkCoords = Chunk::worldToChunk(player->position, chunkSize);
	    glm::ivec3 localCoords = Chunk::worldToLocal(player->position, chunkSize);
	    {
	    ImGui::Begin("DEBUG", NULL, ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_::ImGuiWindowFlags_NoMove | ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);
	    ImGui::Text("FPS: %f", getFPS(deltaTime));
	    RenderFrametimeGraph();
	    ImGui::Text("Draw Calls: %d", g_drawCallCount);
	    ImGui::Text("Player position: %f, %f, %f", player->position.x, player->position.y, player->position.z);
	    ImGui::Text("Player is in chunk: %i, %i, %i", chunkCoords.x, chunkCoords.y, chunkCoords.z);
	    ImGui::Text("Player local position: %d, %d, %d", localCoords.x, localCoords.y, localCoords.z);
	    ImGui::Text("Player velocity: %f, %f, %f", player->velocity.x, player->velocity.y, player->velocity.z);
	    ImGui::Text("Camera position: %f, %f, %f", player->_camera->Position.x, player->_camera->Position.y, player->_camera->Position.z);
	    ImGui::Text("Player MODE: %s", player->getMode());
	    ImGui::Text("Player STATE: %s", player->getState());
	    /*
	    ImGui::Text("is OnGround: %s", player->isOnGround == true ? "TRUE" : "FALSE");
	    ImGui::Text("is Damageable: %s", player->isDamageable == true ? "TRUE" : "FALSE");
	    ImGui::Text("is Running: %s", player->isRunning == true ? "TRUE" : "FALSE");
	    ImGui::Text("is Flying: %s", player->isFlying == true ? "TRUE" : "FALSE");
	    ImGui::Text("is Swimming: %s", player->isSwimming == true ? "TRUE" : "FALSE");
	    ImGui::Text("is Walking: %s", player->isWalking == true ? "TRUE" : "FALSE");
	    ImGui::Text("is Crouched: %s", player->isCrouched == true ? "TRUE" : "FALSE");
	    ImGui::Text("Player can place blocks: %s", player->canPlaceBlocks == true ? "TRUE" : "FALSE");
	    ImGui::Text("Player can break blocks: %s", player->canBreakBlocks == true ? "TRUE" : "FALSE");
	    */
	    ImGui::Text("is Player third-person: %s", player->isThirdPerson == true ? "TRUE" : "FALSE");
	    ImGui::Text("is camera third-person: %s", player->_camera->isThirdPerson == true ? "TRUE" : "FALSE");
	    ImGui::Text("renderSkin: %s", player->renderSkin == true ? "TRUE" : "FALSE");
	    ImGui::Text("Selected block: %s", Block::toString(player->selectedBlock));
	    }

	    if(renderUI && !mouseClickEnabled) {
		ImGui::SliderFloat("Player walking speed ", &player->walking_speed, 0.0f, 100.0f);
		ImGui::SliderFloat("Player flying speed", &player->flying_speed, 0.0f, 100.0f);
		ImGui::SliderFloat("Player running speed increment", &player->running_speed_increment, 0.0f, 100.0f);
		ImGui::SliderInt("Render distance", (int*)&player->render_distance, 0, 30);
		ImGui::SliderFloat("Max Interaction Distance", &player->max_interaction_distance, 0.0f, 100.0f);
		ImGui::SliderFloat("Sensitivity", &player->_camera->MouseSensitivity, 0.0f, 1.5f);
		ImGui::SliderFloat("FOV", &player->_camera->FOV, 0.001f, 179.899f);
		ImGui::SliderFloat("Near Clip Plane", &player->_camera->NEAR_PLANE, 0.001f, 1.5f);
		ImGui::SliderFloat("Far Clip Plane", &player->_camera->FAR_PLANE, 1.5f, 1000000.0f);
		ImGui::SliderFloat("LINE_WIDTH", &LINE_WIDTH, 0.001f, 9.0f);
		ImGui::SliderFloat("GRAVITY", &GRAVITY, -10.0f, 30.0f);
		ImGui::SliderFloat3("BACKGROUND COLOR", &backgroundColor.r, 0.0f, 1.0f);
		ImGui::SliderFloat3("armOffset", &player->armOffset.x, -5.0f, 5.0f);
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
		ImGui::SliderFloat("Camera Distance", &player->_camera->Distance, 1.0f, 20.0f);
		ImGui::Checkbox("renderTerrain", &renderTerrain);
		ImGui::Checkbox("renderPlayer", &player->renderSkin);
		static bool debug = false;
		ImGui::Checkbox("debug", &debug); // Updates the value
		chunkManager->chunkShader->setBool("debug", debug);

	    }
	    ImGui::End();

	    ImGui::Render();
	    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


	}
#endif

	// other UI things...
	



	if(DEPTH_TEST) {
	    glEnable(GL_DEPTH_TEST);
	    glDepthFunc(GL_LEQUAL);
	}
	glPolygonMode(GL_FRONT_AND_BACK, WIREFRAME_MODE ? GL_LINE : GL_FILL);
	// --- Swap Buffers ---
	glfwSwapBuffers(window);
    }
}
// GLFW Callback implementations
void Application::framebuffer_size_callback(GLFWwindow* window, int width, int height) {

    glViewport(0, 0, width, height);

    Application* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (width <= 0 || height <= 0) {
	// Don't recalculate the projection matrix, skip this frame's rendering, or log a warning
	return;
    }
    if (app) {
	app->aspectRatio = static_cast<float>(width) / height;
	app->player->_camera->setAspectRatio(app->aspectRatio);
	app->ui->SetViewportSize(width, height);
    }
}
void Application::cursor_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (!application) return;

    std::pair<double, double> mouseDelta = application->input->getMouseDelta();

    // Conditionally invert Y-axis based on the flag
    float yDelta = application->input->invertYAxis ? static_cast<float>(mouseDelta.second) : static_cast<float>(-mouseDelta.second);

    glm::vec2 delta(static_cast<float>(mouseDelta.first), yDelta);

    application->player->processMouseMovement(delta.x, delta.y, true);
    if (application->ui->context && application->FREE_CURSOR)
	application->ui->context->ProcessMouseMove(static_cast<int>(xpos), static_cast<int>(ypos), application->ui->GetKeyModifiers()); 

}
void Application::mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* application = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (!application) return;

    if (!application->ui->context) return;
    if (action == GLFW_PRESS)
	application->ui->context->ProcessMouseButtonDown(button, mods);
    else if (action == GLFW_RELEASE)
	application->ui->context->ProcessMouseButtonUp(button, mods);

}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (app)
	app->player->processMouseScroll(static_cast<float>(yoffset));
    if (app->ui->context)
	app->ui->context->ProcessMouseWheel(static_cast<float>(-yoffset), 0);
}
void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

    if (!app) return;
    if (!app->ui || !app->ui->context) return;
    // Update modifier state
    bool pressed = (action != GLFW_RELEASE);
    switch (key) {
	case GLFW_KEY_LEFT_SHIFT:
	case GLFW_KEY_RIGHT_SHIFT:
	    app->ui->isShiftDown = pressed;
	    break;
	case GLFW_KEY_LEFT_CONTROL:
	case GLFW_KEY_RIGHT_CONTROL:
	    app->ui->isCtrlDown = pressed;
	    break;
	case GLFW_KEY_LEFT_ALT:
	case GLFW_KEY_RIGHT_ALT:
	    app->ui->isAltDown = pressed;
	    break;
	case GLFW_KEY_LEFT_SUPER:
	case GLFW_KEY_RIGHT_SUPER:
	    app->ui->isMetaDown = pressed;
	    break;
	default:
	    break;
    }

    // Map and pass the key event to RmlUI
    Rml::Input::KeyIdentifier rml_key = app->ui->MapKey(key);
    if (action == GLFW_PRESS)
	app->ui->context->ProcessKeyDown(rml_key, app->ui->GetKeyModifiers());
    else if (action == GLFW_RELEASE)
	app->ui->context->ProcessKeyUp(rml_key, app->ui->GetKeyModifiers());
}

void Application::MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar* message, const void* userParam) {

    (void)length;
    (void)userParam;
    // Choose color based on severity
    const char* severityColor = RESET_COLOR; // Default to no color
    const char* severityText = "Unknown";

    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            severityColor = RED;
            severityText = "High";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severityColor = YELLOW;
            severityText = "Medium";
            break;
        case GL_DEBUG_SEVERITY_LOW:
	    return;
            severityColor = GREEN;
            severityText = "Low";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
	    return;
            severityColor = CYAN;
            severityText = "Notification";
            break;
    }

    // Output the debug message with color coding for severity
    std::cerr << severityColor << "OpenGL Debug Message: " << message << RESET_COLOR << "\n";

    // Print additional information
    std::cerr << "Source: ";
    switch (source) {
        case GL_DEBUG_SOURCE_API: std::cerr << "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: std::cerr << "Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cerr << "Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: std::cerr << "Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION: std::cerr << "Application"; break;
        case GL_DEBUG_SOURCE_OTHER: std::cerr << "Other"; break;
    }
    std::cerr << "\n";

    std::cerr << "Type: ";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: std::cerr << "Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cerr << "Deprecated Behavior"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: std::cerr << "Undefined Behavior"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: std::cerr << "Performance"; break;
        case GL_DEBUG_TYPE_MARKER: std::cerr << "Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: std::cerr << "Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP: std::cerr << "Pop Group"; break;
    }
    std::cerr << "\n";

    std::cerr << "Severity: " << severityText << "\n";

    std::cerr << "Message ID: " << id << "\n";
}
