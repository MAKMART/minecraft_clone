#pragma once
class ECS;
class ChunkManager;
struct Camera;
struct FrustumVolume;
struct FramebufferManager;

void chunk_renderer_system(ECS& ecs, const ChunkManager& cm, Camera* cam, FrustumVolume& wanted_fv, FramebufferManager& fb);
