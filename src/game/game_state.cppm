module;
#include <gl.h>
export module game_state;

import std;
import ecs;
import ecs_components;
import player;
import glm;
import game_factories;

export struct game_state {

  public:
    ActiveCamera *get_active_camera_component() {
      // BEWARE could be nullptr if no ActiveCamera component was found
      return ecs.get_component<ActiveCamera>(camera_manager_entity);
    }
    game_state(int width, int height)
    {
      player = create_player(ecs, {0.1f, 50.0f, 0.1f});
      camera_manager_entity = ecs.create_entity();
      ecs.emplace_component<ActiveCamera>(camera_manager_entity, player.camera);
#if defined(DEBUG)
      debug_cam = ecs.create_entity();
      ecs.emplace_component<Camera>(debug_cam);
      ecs.emplace_component<Transform>(debug_cam, glm::vec3(0.0f, 10.0f, 0.0f));
      // At the start we do not need to add these componets since they will change at runtime when the player will switch to the debug camera
      //ecs.add_component(debug_cam, InputComponent{});
      ecs.emplace_component<Velocity>(debug_cam);
      ecs.emplace_component<MovementIntent>(debug_cam);
      ecs.emplace_component<DebugCamera>(debug_cam);
      ecs.emplace_component<DebugCameraController>(debug_cam);
      ecs.emplace_component<RenderTarget>(debug_cam, RenderTarget(width, height, {
            { framebuffer_attachment_type::color, GL_RGBA16F }, // albedo
                                                                //{ framebuffer_attachment_type::color, GL_RGBA16F }, // normal
            { framebuffer_attachment_type::color, GL_RG16F   }, // material
            { framebuffer_attachment_type::depth, GL_DEPTH_COMPONENT24 }
            }));
#endif
    }
  enum class game_mode : std::uint8_t {
    MAIN_MENU = 0,
    IN_GAME = 1,
    LOADING_SCREEN = 2,
    PAUSED = 3
  };
  game_mode mode = game_mode::MAIN_MENU;
  // remember that C++ initializes in the order of member declaration
  ECS ecs;
  bool render_ui = true;
  bool render_terrain = true;
  Entity camera_manager_entity;
  Entity debug_cam;
  Player player;
};
