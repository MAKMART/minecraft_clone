export module ecs_systems;

import ecs;
import ecs_components;
import framebuffer_manager;
import chunk_manager;

export {
	void camera_controller_system(ECS& ecs, Entity player);
	void chunk_renderer_system(ECS& ecs, ChunkManager& cm, Camera* cam, FrustumVolume& wanted_fv, FramebufferManager& fb);
	void input_system(ECS& ecs);
	void movement_intent_system(ECS& ecs, const Camera* cam);
	void movement_physics_system(ECS& ecs, ChunkManager& chunkManager, float dt);
	void camera_system(ECS& ecs);
	void debug_camera_system(ECS& ecs, float dt);
	void frustum_volume_system(ECS& ecs);
	void camera_temporal_system(ECS& ecs);
}
