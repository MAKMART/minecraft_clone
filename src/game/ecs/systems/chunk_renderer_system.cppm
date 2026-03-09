export module chunk_renderer_system;

import ecs;
import ecs_components;
import framebuffer_manager;
import chunk_manager;

export void chunk_renderer_system(ECS& ecs, ChunkManager& cm, Camera* cam, FrustumVolume& wanted_fv, FramebufferManager& fb);
