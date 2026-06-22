export module ecs_systems;

import std;
import ecs;
import ecs_components;
import framebuffer_manager;
import chunk_manager;
import frame_context;
import input_manager;

export {
	void camera_pose_system(ECS& ecs, Entity player, const frame_context& ctx, float dt, const InputManager& input);
  void chunk_renderer_system(ECS& ecs, ChunkManager& cm, Entity cam, const FrustumVolume& wanted_fv, const FramebufferManager& fb);
	void input_system(ECS& ecs, const InputManager& input);
	void movement_intent_system(ECS& ecs, const Entity camera);
  void player_state_system(ECS& esc);
	void movement_physics_system(ECS& ecs, ChunkManager& chunkManager, float dt);
	void frustum_volume_system(ECS& ecs);
	void camera_temporal_system(ECS& ecs, const frame_context& ctx);
  // TODO: Implement this function |
  //                               V
  void world_interaction_system(ECS& ecs, ChunkManager& manager);
}
