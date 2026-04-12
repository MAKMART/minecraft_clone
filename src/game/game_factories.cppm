module;
#include <glad/glad.h>
export module game_factories;

import player;
import ecs;
import ecs_components;
import glm;

export Player create_player(ECS& ecs, glm::vec3 spawn_pos)
{
  Player p;
  p.playerHeight = 1.8f;
	p.self = ecs.create_entity();
	ecs.emplace_component<Transform>(p.self, spawn_pos);
	ecs.emplace_component<Velocity>(p.self);
	ecs.emplace_component<Collider>(p.self, spawn_pos, glm::vec3(0.3f, p.playerHeight / 2.0f, 0.3f));
	ecs.emplace_component<InputComponent>(p.self);
	ecs.emplace_component<MovementIntent>(p.self);
	ecs.emplace_component<MovementConfig>(p.self, MovementConfig{
			.can_jump = true,
			.can_walk = true,
			.can_run = true,
			.can_crouch = true,
			.can_fly = false,
			.jump_height = 1.252f,
			.walk_speed = 3.6f,
			.run_speed = 777.8f,
			.crouch_speed = 2.5f,
			.fly_speed = 17.5f
			});
	ecs.emplace_component<PlayerState>(p.self);
	ecs.emplace_component<PlayerMode>(p.self);

	p.eyeHeight = p.playerHeight * 0.9f;

	p.camera = ecs.create_entity();
	ecs.emplace_component<Transform>(p.camera);
	ecs.emplace_component<Camera>(p.camera);
	ecs.emplace_component<CameraController>(p.camera, p.self, glm::vec3(0.0f, p.eyeHeight / 2.0f, 0.0f));
	ecs.emplace_component<FrustumVolume>(p.camera);
	ecs.emplace_component<CameraTemporal>(p.camera);
	ecs.emplace_component<RenderTarget>(p.camera, RenderTarget({
				{ framebuffer_attachment_type::color, GL_RGBA16F }, // albedo
																	//{ framebuffer_attachment_type::color, GL_RGBA16F }, // normal
																	//{ framebuffer_attachment_type::color, GL_RG16F   }, // material
				{ framebuffer_attachment_type::depth, GL_DEPTH_COMPONENT24 }
				}));
  return p;

}
