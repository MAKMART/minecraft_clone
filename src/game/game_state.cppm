module;
#include <gl.h>
export module game_state;

import std;
import ecs;
import ecs_components;
import player;
import glm;
import game_factories;
import logger;

export struct game_state {

  public:
    // This is a component to handle which camera is currently active
    struct ActiveCamera {
      Entity target;
    };
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
      debug_cam = create_debug_camera(ecs, {0.0f, 10.0f, 0.0f});
#endif


      log::info("Player entity id: {}", player.self.id);
      log::info("camera_manager_entity entity id: {}", camera_manager_entity.id);
#if defined(DEBUG)
      log::info("Debug Camera entity id: {}", debug_cam.id);
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
