export module movement_physics_system;

import ecs;
import ecs_components;
import chunk_manager;
import glm;

// Optimized collision check: integer-based, minimal floating-point, early exits
bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager);

export void movement_physics_system(ECS& ecs, ChunkManager& chunkManager, float dt);
