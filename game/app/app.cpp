module;
#if defined(TRACY_ENABLE)
#include "tracy/Tracy.hpp"
#endif
#include <stb_image.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
module app;
// TODO: fix the damn stencil buffer to allow cropping in RmlUI

// TODO:    FIX THE CAMERA CONTROLLER TO WORK WITH INTERPOLATION

import core;
import logger;
import ecs;
import ecs_components;
import vertex_buffer;
import index_buffer;
import gl_state;
import timer;
import ecs_systems;
import glm;
import ui;
import debug_drawer;
import raycast;
import aabb;
import debug;
import framebuffer_manager;

App::App(int width, int height)
  : win_context(std::make_unique<WindowContext>(width, height, std::string(PROJECT_NAME) + std::string(" ") + std::string(PROJECT_VERSION))),
  fb_manager(*win_context.get()),
  ui(width, height, MAIN_FONT_DIRECTORY),
  g_state(width, height),
  playerShader("Player", PLAYER_VERTEX_SHADER_DIRECTORY, PLAYER_FRAGMENT_SHADER_DIRECTORY),
  fb_debug("FB_DEBUG", SHADERS_DIRECTORY / "fb_vert.glsl", SHADERS_DIRECTORY / "fb_debug_frag.glsl"),
  fb_player("FB_PLAYER", SHADERS_DIRECTORY / "fb_vert.glsl", SHADERS_DIRECTORY / "fb_player_frag.glsl"),
  Atlas(BLOCK_ATLAS_TEXTURE_DIRECTORY, GL_RGBA, GL_REPEAT, GL_NEAREST),
  crossHairshader("Crosshair", CROSSHAIR_VERTEX_SHADER_DIRECTORY, CROSSHAIR_FRAGMENT_SHADER_DIRECTORY),
  crossHairTexture(ICONS_DIRECTORY, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST)
{
  /*
     int nExt;
     glGetIntegerv(GL_NUM_EXTENSIONS, &nExt);
     for (int i = 0; i < nExt; ++i) {
     std::cout << glGetStringi(GL_EXTENSIONS, i) << std::endl;
     }
   */

  // This is temporary because logging output is getting redirected to an imgui window
  log::enable_colors(false);

  glfwSetWindowUserPointer(win_context->window, this);

  {
    GLFWwindow* win = win_context->window;
    glfwSetKeyCallback(win,
        [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->on_key_callback(key, scancode, action, mods);
        }
        );
    glfwSetMouseButtonCallback(win,
        [](GLFWwindow* window, int button, int action, int mods) {
        auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->on_mouse_button_callback(button, action, mods);
        });
    glfwSetCursorPosCallback(win,
        [](GLFWwindow* window, double xpos, double ypos) {
        auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->on_cursor_pos_callback(xpos, ypos);
        });
    glfwSetScrollCallback(win,
      [](GLFWwindow* window, double xoffset,  double yoffset) {
        auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        app->on_scroll_callback(xoffset, yoffset);
      });
    glfwSetWindowFocusCallback(win,
        [](GLFWwindow* window, int focused) {
        auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        if (focused) {
        app->on_window_focus_callback(true);
        } else {
        app->on_window_focus_callback(false);
        }
        });
    glfwSetFramebufferSizeCallback(win_context->window,
        [](GLFWwindow* window, int width, int height) {
        auto* app = static_cast<App*>(glfwGetWindowUserPointer(window));
        if (!app) return;
        if (width <= 0 || height <= 0) return;
        app->on_framebuffer_resize(width, height);
        }
        );
  }
  input.setContext(win_context.get());
  input.setMouseTrackingEnabled(true);
  // Cubemap
  glGenTextures(1, &cube_id);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cube_id);


  int __width, __height, nrChannels;
  unsigned char *data;
  for(unsigned int i = 0; i < 6; i++)
  {
    std::string string = ASSETS_DIRECTORY / "skybox" / cube_faces[i];
    //log::info("loading cubemap texture from: {}", string);
    data = stbi_load(string.c_str(), &__width, &__height, &nrChannels, 0);
    if (data) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
          __width, __height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data); // BEWARE OF THE FORMAT: It has to match the provided images
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
  g_state.ecs.for_each_component<RenderTarget>([&](Entity e, RenderTarget& rt) {
      fb_manager.ensure(e, rt);
      });
  // CROSSHAIR STUFF
  float crosshairSize;
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
  // IMGUI SETUP
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  // ImGuiIO& io = ImGui::GetIO();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(win_context->window, true);
  ImGui_ImplOpenGL3_Init("#version 460");

	glCreateVertexArrays(1, &VAO);

	last_frame_time = glfwGetTime();
}
App::~App()
{
	glDeleteVertexArrays(1, &crosshairVAO);
  glDeleteVertexArrays(1, &VAO);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
void App::on_framebuffer_resize(int width, int height) noexcept {
  if(width <= 0 || height <= 0) return;
  win_context->set_framebuffer_size(width, height);
  GLState::set_viewport(0, 0, width, height);
  assert(frame_ctx.active_camera.is_valid());
  ImGui::SetNextWindowPos(ImVec2(width, height), ImGuiCond_Always);
  g_state.ecs.get_component<Camera>(frame_ctx.active_camera)->aspect_ratio = float(static_cast<float>(win_context->get_framebuffer_width()) / win_context->get_framebuffer_height());

  g_state.ecs.for_each_component<RenderTarget>([&](const Entity e, RenderTarget& rt) {
      fb_manager.ensure(e, rt);
      log::info("rt.width = {}, rt.height = {}", rt.width, rt.height);
      ui.SetViewportSize(win_context->get_framebuffer_width(), win_context->get_framebuffer_height());
      }
      );
}
void App::run() noexcept
{
  //           #     #    #    ### #     #     #####     #    #     # #######    #       ####### ####### ######
  //           ##   ##   # #    #  ##    #    #     #   # #   ##   ## #          #       #     # #     # #     #
  //           # # # #  #   #   #  # #   #    #        #   #  # # # # #          #       #     # #     # #     #
  //           #  #  # #     #  #  #  #  #    #  #### #     # #  #  # #####      #       #     # #     # ######
  //           #     # #######  #  #   # #    #     # ####### #     # #          #       #     # #     # #
  //           #     # #     #  #  #    ##    #     # #     # #     # #          #       #     # #     # #
  //           #     # #     # ### #     #     #####  #     # #     # #######    ####### ####### ####### #
  while (!win_context->should_close()) {
    begin_frame();
    update();
    render();
    end_frame();
  }
}
void App::begin_frame() noexcept
{
		double current_time = glfwGetTime();
    frame_ctx.delta_time = current_time - last_frame_time;
    frame_ctx.first_frame = frame_ctx.frame_number == 0;  // The fist frame has idx 0x0000000
    last_frame_time = current_time;
    g_drawCallCount = 0;
    // Clear the screen for the start of the new current frame
    GLState::set_depth_test(true);
    GLState::set_wireframe(false);
    GLState::clear_depth();
    glStencilMask(0xFF); // Ensure we can actually clear the stencil bits
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


    // Build this frame's active camera
    frame_ctx.active_camera = g_state.get_active_camera_component()->target;
    {
      Camera* cam = g_state.ecs.get_component<Camera>(frame_ctx.active_camera);
      if (!cam )
        log::error("Couldn't find active cam for frame {}", frame_ctx.frame_number);
      frame_ctx.cam = cam;

    }
    // Build matrices

    Transform* transform = g_state.ecs.get_component<Transform>(frame_ctx.active_camera);
    if (transform) {
      transform->rot = glm::normalize(transform->rot);
    } else if (!transform) {
      log::error("Couldn't find a Transform component on this frame's active_camera");
    }
    frame_ctx.view_matrix = frame_ctx.cam->view_matrix(*transform);
    frame_ctx.projection_matrix = frame_ctx.cam->projection_matrix();
    glm::mat4 view_proj_matrix = frame_ctx.projection_matrix * frame_ctx.view_matrix;
    frame_ctx.view_proj_matrix = view_proj_matrix;
    frame_ctx.inv_view_proj_matrix = glm::inverse(view_proj_matrix);

}
void App::update() noexcept
{
  UpdateFrametimeGraph(frame_ctx.delta_time);
  // --- Input & Event Processing ---
  glfwPollEvents();     // new events arrive → PRESSED/RELEASED, mouse, scroll
  input_system(g_state.ecs, input);

  { // Switch currently active camera
    auto* active_camera = g_state.get_active_camera_component();

    if (input.isPressed(GLFW_KEY_RIGHT)) {
      active_camera->target = g_state.player.camera;
      g_state.ecs.add_component_if_missing<InputComponent>(g_state.player.self, InputComponent{});
    }

    if (input.isPressed(GLFW_KEY_LEFT)) {
#if defined(DEBUG)
      active_camera->target = g_state.debug_cam;
      g_state.ecs.add_component_if_missing<InputComponent>(g_state.debug_cam, InputComponent{});
#endif
    }

  }

  if (input.isPressed(EXIT_KEY))
    glfwSetWindowShouldClose(win_context->window, true);

  if (input.isPressed(FULLSCREEN_KEY))
    win_context->toggle_fullscreen();

  static float scale = 0.01f;
  if (frame_ctx.active_camera == g_state.player.camera) {
    //fb_player.setFloat("near_plane", cam->near_plane);
    //fb_player.setFloat("far_plane", cam->far_plane);
    fb_player.setFloat("scale", scale);
  }
#if defined(DEBUG)
  else if (frame_ctx.active_camera == g_state.debug_cam) {
    fb_debug.setFloat("near_plane", frame_ctx.cam->near_plane);
    fb_debug.setFloat("far_plane", frame_ctx.cam->far_plane);
  }
#endif
  if (input.isPressed(GLFW_KEY_UP))
    scale+=0.001f;
  if (input.isPressed(GLFW_KEY_DOWN))
    scale-=0.001f;

  static bool toggle = false;
  static bool was_pressed = false;

  bool pressed = input.isPressed(GLFW_KEY_6);

  if (pressed && !was_pressed) { // only trigger on key-down edge
    toggle = !toggle;          // flip toggle
    if (frame_ctx.active_camera == g_state.player.camera) {
      fb_player.setInt("toggle", toggle ? 1 : 0);
    }
#if defined(DEBUG)
    else if (frame_ctx.active_camera == g_state.debug_cam) {
      fb_debug.setInt("toggle", toggle ? 1 : 0);
    }
#endif
  }

  was_pressed = pressed; // remember state for next frame


  // 1. Player
  if (input.isMouseTrackingEnabled()) {
    // if (input.isMousePressed(ATTACK_BUTTON) && g_state.player.canBreakBlocks && frame_ctx.active_camera == g_state.player.camera) {
    // 	g_state.player.breakBlock(manager);
    // }
    // if (input.isMousePressed(DEFENSE_BUTTON) && g_state.player.canPlaceBlocks && frame_ctx.active_camera == g_state.player.camera) {
    // 	g_state.player.placeBlock(manager);
    // }
#if defined(DEBUG)
    if (frame_ctx.active_camera == g_state.debug_cam) {
      Transform* trans = g_state.ecs.get_component<Transform>(frame_ctx.active_camera);
      if (!trans)
        log::error("NO TRANSFORM FOR ENTITY: {}", frame_ctx.active_camera.id);
      glm::vec4 clip = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

      glm::vec4 view_space = glm::inverse(frame_ctx.projection_matrix) * clip;
      view_space.z = 1.0f; // forward direction
      view_space.w = 0.0f;  // this is a direction, not a position

      glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(frame_ctx.view_matrix) * view_space));
      glm::vec3 ray_origin = trans->pos; // start at camera position
      if (input.isMouseHeld(ATTACK_BUTTON)) {
        std::optional<glm::ivec3> hitBlock = raycast::voxel(manager, ray_origin, ray_dir, g_state.player.max_interaction_distance);
        if (hitBlock.has_value()) {
          glm::ivec3 blockPos = hitBlock.value();
          manager.update_block(blockPos, Block::blocks::AIR);
        }
        // log::info("Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);
      }
      if (input.isMouseHeld(DEFENSE_BUTTON)) {
        std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycast::voxel_normals(manager, ray_origin, ray_dir, g_state.player.max_interaction_distance);
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
             manager.updateBlock(placePos + glm::ivec3(i, j, k), static_cast<Block::blocks>(g_state.player.selectedBlock));
             }
             }
             */
          manager.update_block(placePos, static_cast<Block::blocks>(g_state.player.selectedBlock));
        }
      }
    }
#endif


    float scrollY = input.getScroll().y;
    if (scrollY != 0.0f) {
      CameraController* ctrl = g_state.ecs.get_component<CameraController>(frame_ctx.active_camera);
      float scroll_speed_multiplier = 1.0f;
      if (ctrl && !ctrl->third_person) {
        g_state.player.selectedBlock += static_cast<int>(scrollY * scroll_speed_multiplier);
        if (g_state.player.selectedBlock < 1)
          g_state.player.selectedBlock = 1;
        if (g_state.player.selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS))
          g_state.player.selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
      }

      // To change FOV
      // camCtrl.processMouseScroll(scrollY);
    }
  }

  PlayerMode* mode = g_state.ecs.get_component<PlayerMode>(g_state.player.self);
  PlayerState* player_state = g_state.ecs.get_component<PlayerState>(g_state.player.self);
  MovementConfig* player_movement_config = g_state.ecs.get_component<MovementConfig>(g_state.player.self);
  if (input.isPressed(CAMERA_SWITCH_KEY) && g_state.ecs.has_component<CameraController>(frame_ctx.active_camera)) {
    g_state.ecs.get_component<CameraController>(frame_ctx.active_camera)->third_person = !g_state.ecs.get_component<CameraController>(frame_ctx.active_camera)->third_person;
  }
  // MovementConfig survival_config {
  //   .can_jump = true,
  //   .can_walk = true,
  //   .can_run = true,
  //   .can_crouch = true,
  //   .can_fly = false,
  //   .jump_height = 1.8f,
  //   .walk_speed = 5.0f,
  //   .run_speed = 7.5f,
  //   .crouch_speed = 2.5f,
  //   .fly_speed = 10.0f
  // };
  //
  // MovementConfig creative_config {
  //   .can_jump = true,
  //   .can_walk = true,
  //   .can_run = true,
  //   .can_crouch = true,
  //   .can_fly = false,
  //   .jump_height = 1.8f,
  //   .walk_speed = 5.0f,
  //   .run_speed = 7.5f,
  //   .crouch_speed = 2.5f,
  //   .fly_speed = 10.0f
  // };
  //
  // MovementConfig spectator_config {
  //   .can_jump = true,
  //   .can_walk = true,
  //   .can_run = true,
  //   .can_crouch = true,
  //   .can_fly = false,
  //   .jump_height = 1.8f,
  //   .walk_speed = 5.0f,
  //   .run_speed = 7.5f,
  //   .crouch_speed = 2.5f,
  //   .fly_speed = 10.0f
  // };
  //
  if (input.isPressed(SURVIVAL_MODE_KEY)) {
    mode->mode = ModeType::SURVIVAL;
  }

  if (input.isPressed(CREATIVE_MODE_KEY)) {
    mode->mode = ModeType::CREATIVE;
  }

  if (input.isPressed(SPECTATOR_MODE_KEY)) {
    mode->mode = ModeType::SPECTATOR;
  }

  // Reload shaders
  if (input.isPressed(GLFW_KEY_H)) {
    // manager.getShader().reload();
    playerShader.reload();
    fb_debug.reload();
    fb_player.reload();
    playerShader.reload();
    crossHairshader.reload();
  }

  if (input.isHeld(GLFW_KEY_P))
    crosshair_size += 0.1f;
  if (input.isHeld(GLFW_KEY_M))
    crosshair_size -= 0.1f;

#if defined(DEBUG)
  if (input.isPressed(GLFW_KEY_ENTER) && frame_ctx.active_camera == g_state.debug_cam) {
    g_state.ecs.get_component<Transform>(g_state.player.self)->pos = g_state.ecs.get_component<Transform>(g_state.debug_cam)->pos;
    g_state.ecs.get_component<Velocity>(g_state.player.self)->value = g_state.ecs.get_component<Velocity>(g_state.debug_cam)->value;
  }
#endif

  // 2. UI
  if (ui.get_context()) {
    glm::vec2 mousePos = input.getMousePos();
    ui.get_context()->ProcessMouseMove(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y), ui.GetKeyModifiers(input));

    // Mouse buttons
    for (int b = 0; b < InputManager::MAX_MOUSE_BUTTONS; ++b) {
      if (input.isMousePressed(b))
        ui.get_context()->ProcessMouseButtonDown(b, 0);
      else if (input.isMouseReleased(b))
        ui.get_context()->ProcessMouseButtonUp(b, 0);
    }

    // Scroll
    float scrollY = input.getScroll().y;
    if (scrollY != 0.0f)
      ui.get_context()->ProcessMouseWheel(-scrollY, 0);

    // Keyboard
    for (int k = 0; k < InputManager::MAX_KEYS; ++k) {
      if (input.isPressed(k))
        ui.get_context()->ProcessKeyDown(ui.MapKey(k), ui.GetKeyModifiers(input));
      else if (input.isReleased(k))
        ui.get_context()->ProcessKeyUp(ui.MapKey(k), ui.GetKeyModifiers(input));
    }
  }

#if defined(DEBUG)
  if (input.isPressed(GLFW_KEY_8)) {
    frame_ctx.debug_render = !frame_ctx.debug_render;
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




  CameraController* ctrl = g_state.ecs.get_component<CameraController>(g_state.player.camera);

  movement_intent_system(g_state.ecs, frame_ctx.active_camera);
  player_state_system(g_state.ecs);
  movement_physics_system(g_state.ecs, manager, frame_ctx.delta_time);
  camera_pose_system(g_state.ecs, g_state.player.self, frame_ctx, frame_ctx.delta_time, input);
  frustum_volume_system(g_state.ecs);


#if defined(DEBUG)
  if (frame_ctx.debug_render) {
    auto* player_trans = g_state.ecs.get_component<Transform>(g_state.player.self);
    auto aabb = g_state.ecs.get_component<Collider>(g_state.player.self)->get_AABB_at(player_trans->pos);
    DebugDrawer::get().add_aabb(aabb, glm::vec3(0.3f, 1.0f, 0.5f));
    Transform* player_cam_trans = g_state.ecs.get_component<Transform>(g_state.player.camera);
    Transform* trans = g_state.ecs.get_component<Transform>(g_state.player.self);
    DebugDrawer::get().add_ray(trans->pos, player_cam_trans->forward(), {1.0f, 0.0f, 0.0f});
    DebugDrawer::get().add_ray(trans->pos, player_cam_trans->up(), {0.0f, 1.0f, 0.0f});
    DebugDrawer::get().add_ray(trans->pos, player_cam_trans->right(), {0.0, 0.0f, 1.0f});

    g_state.ecs.for_each_components<Camera, Transform>([&](Entity e, Camera, Transform& trans){
        DebugDrawer::get().add_aabb(AABB::fromCenterSize(trans.pos, {0.5f, 0.8f, 0.5f}), glm::vec3(1.0f, 0.0f, 1.0f));
        });

    float ndc_z = -1.0f; // near plane
    glm::vec4 clip = glm::vec4(0.0f, 0.0f, ndc_z, 1.0f);

    Camera* player_cam = g_state.ecs.get_component<Camera>(g_state.player.camera);
    glm::vec4 view_space = glm::inverse(player_cam->projection_matrix()) * clip;
    view_space.z = -1.0f; // forward direction
    view_space.w = 0.0f;  // this is a direction, not a position

    glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(player_cam->view_matrix(*player_cam_trans)) * view_space));
    glm::vec3 cam_pos   = player_cam_trans->pos;
    DebugDrawer::get().add_ray(cam_pos, ray_dir, {1.0f, 0.0f, 0.0f});
    DebugDrawer::get().add_ray(cam_pos, player_cam_trans->forward(), {1.0f, 0.0f, 0.0f});


    // Add all chunks' bounding boxes
    // for (const auto& [chunkKey, chunkPtr] : manager.get_all()) {
    // 	if (!chunkPtr)
    // 		continue; // safety
    //
    // 	// Color for chunk boxes, maybe a translucent blue-ish?
    // 	DebugDrawer::get().add_aabb(chunkPtr->aabb, glm::vec3(0.3f, 0.5f, 1.0f));
    // }
    auto vel = g_state.ecs.get_component<Velocity>(g_state.player.self)->value;
    DebugDrawer::get().add_ray(trans->pos, vel, {1.0f, 1.0f, 0.0f});
    DebugDrawer::get().add_ray(trans->pos, glm::normalize(glm::vec3{0.0f, -GRAVITY, 0.0f}), glm::vec3(0.5f, 0.5f, 1.0f));
  }
#endif


  manager.generate_chunks(g_state.ecs.get_component<Transform>(g_state.player.self)->pos, g_state.player.render_distance);

}
void App::render() noexcept
{

  // -- Render Player -- (BEFORE UI pass)
  // TODO: Actually fix and implement this shit
  /*
     if (player.renderSkin) {
     playerShader.setMat4("projection", cam->projectionMatrix);
     playerShader.setMat4("view", cam->viewMatrix);
     player.render(playerShader);
     }
     */


  if (g_state.render_terrain) {
    //manager.getShader().checkAndReloadIfModified();
    // manager.getShader().setMat4("projection", frame_ctx.cam->projectionMatrix);
    // manager.getShader().setMat4("view", frame_ctx.cam->viewMatrix);
    //manager.getShader().setFloat("time", (float)glfwGetTime());

    manager.getShader2().setMat4("u_projection", frame_ctx.projection_matrix);
    manager.getShader2().setMat4("u_view", frame_ctx.view_matrix);
    Transform* cam_trans = g_state.ecs.get_component<Transform>(frame_ctx.active_camera);
    manager.getShader2().setVec3("eye_position", cam_trans->pos);

    // manager.getShader2().setIVec3("eye_position_int", glm::ivec3(floor(cam_trans->pos)));
    Atlas.Bind(0);

    manager.getShader2().use();
    // Whatever VAO'll work
    glBindVertexArray(VAO);
    chunk_renderer_system(g_state.ecs, manager, frame_ctx.active_camera, *g_state.ecs.get_component<FrustumVolume>(g_state.player.camera), fb_manager);
    glBindVertexArray(0);
    Atlas.Unbind(0);
  }

  // Debug render

#if defined(DEBUG)
  if (frame_ctx.debug_render) {
    GLState::set_depth_test(false);
    GLState::set_face_culling(false);
    DebugDrawer::get().draw(frame_ctx.view_proj_matrix);
  }
#endif

  auto& cur_fb = fb_manager.get(frame_ctx.active_camera);
  glBindTextureUnit(0, cur_fb.color_attachment(0));
  glBindTextureUnit(1, cur_fb.depth_attachment());
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_CUBE_MAP, cube_id);

  if (frame_ctx.active_camera == g_state.player.camera) {
    glm::mat4 prev_view_proj = g_state.ecs.get_component<CameraTemporal>(g_state.player.camera)->prev_view_proj;
    fb_player.setMat4("curr_inv_view_proj", frame_ctx.inv_view_proj_matrix);
    fb_player.setMat4("prev_view_proj", prev_view_proj);
    fb_player.setMat4("invView", glm::inverse(frame_ctx.view_matrix));
    fb_player.setMat4("invProj", glm::inverse(frame_ctx.projection_matrix));
    fb_player.setInt("color", 0);
    fb_player.setInt("depth", 1);
    fb_player.setInt("skybox", 2);
    fb_player.use();
  }
#if defined(DEBUG)
  else if (frame_ctx.active_camera == g_state.debug_cam) {
    fb_debug.setInt("color", 0);
    fb_debug.setInt("depth", 1);
    fb_debug.use();
  }
#endif

  // NOTE: Make sure to have a VAO bound when making this draw call!!
  // Fullscreen triangle covering the whole screen for post-processing and shit like that
  framebuffer::bind_default_draw();
  GLState::set_depth_test(false);
  // GLState::set_wireframe(false);
  glDepthMask(GL_FALSE);
  glBindVertexArray(VAO);
  DrawArraysWrapper(GL_TRIANGLES, 0, 3);

  glBindVertexArray(0);
  glDepthMask(GL_TRUE);

  // --- UI Pass --- (now rendered BEFORE ImGui)
  if (g_state.render_ui/* && frame_ctx.active_camera == player.camera*/) {
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

    if (frame_ctx.active_camera == g_state.player.camera)
      ui.render();
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
  ImGui::Text("FPS: %f", getFPS(frame_ctx.delta_time));
  ImGui::Text("Selected block: ");
  ImGui::SameLine();
  ImGui::SetWindowFontScale(1.2f);
  ImGui::Text(Block::toString(static_cast<Block::blocks>(g_state.player.selectedBlock)));
  ImGui::SetWindowFontScale(1.0f);
  ImGui::SliderInt("Render distance", (int*)&g_state.player.render_distance, 0, 30);
  ImGui::End();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  GLState::sync();
#endif

#if defined(DEBUG)
  if (g_state.render_ui) {
    CameraController* ctrl = g_state.ecs.get_component<CameraController>(g_state.player.camera);
    PlayerState* player_state = g_state.ecs.get_component<PlayerState>(g_state.player.self);
    GLState::set_depth_test(false);
    GLState::set_wireframe(false);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    setup_imgui(g_state, frame_ctx, input, manager, ctrl, player_state);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    GLState::sync();
  }
#endif
}
void App::end_frame() noexcept
{
  camera_temporal_system(g_state.ecs, frame_ctx);
  // TODO: Fix input handling in the manager
  input.update(); // reset this frame's state

  glfwSwapBuffers(win_context->window);
#if defined(TRACY_ENABLE)
  FrameMark;
#endif
  frame_ctx.frame_number++;
}
