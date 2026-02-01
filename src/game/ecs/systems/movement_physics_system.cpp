#include "game/ecs/systems/movement_physics_system.hpp"
#include "core/defines.hpp"
#include "core/logger.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/velocity.hpp"
#include "game/ecs/components/collider.hpp"
#include "game/ecs/components/player_state.hpp"
#include "game/ecs/components/movement_config.hpp"
#include "game/ecs/components/movement_intent.hpp"
#include "chunk/chunk_manager.hpp"
#include <glm/gtx/norm.hpp>
#include "core/aabb.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

// Optimized collision check: integer-based, minimal floating-point, early exits
inline bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager)
{
#if defined(TRACY_ENABLE)
    ZoneScopedN("isCollidingAt");
#endif

    const AABB box = col.getBoundingBoxAt(pos);

    // Integer bounds (inclusive)
    const glm::ivec3 minBlock = glm::floor(box.min);
    const glm::ivec3 maxBlock = glm::floor(box.max - glm::vec3(1e-4f)); // Avoid off-by-one on exact edges

	for (int y = minBlock.y; y <= maxBlock.y; ++y) {
        for (int x = minBlock.x; x <= maxBlock.x; ++x) {
            for (int z = minBlock.z; z <= maxBlock.z; ++z) {
                const glm::ivec3 blockWorldPos(x, y, z);

                Chunk* chunk = chunkManager.getChunk(blockWorldPos);
                if (!chunk)
                    continue; // Treat missing chunks as air (or return true for "void" if preferred)

                const glm::ivec3 local = Chunk::world_to_local(blockWorldPos);
                const Block& block = chunk->get_block(local);

                if (block.type != Block::blocks::AIR &&
                    block.type != Block::blocks::WATER &&
                    block.type != Block::blocks::LAVA) {
                    return true;
                }
            }
        }
    }
    return false;
}

void movement_physics_system(ECS& ecs, ChunkManager& chunkManager, float dt)
{
#if defined(TRACY_ENABLE)
    ZoneScopedN("movement_physics_system");
#endif

    ecs.for_each_components<MovementIntent, Velocity, Collider, PlayerState, MovementConfig, Transform>(
        [&](Entity e, MovementIntent& intent, Velocity& vel, Collider& col, PlayerState& ps, MovementConfig& cfg, Transform& trans) {
#if defined(TRACY_ENABLE)
            ZoneScopedN("Player Physics Tick");
#endif

            // --- Apply input to velocity ---
            float speed = 0.0f;
            switch (ps.current) {
                case PlayerMovementState::Walking:   speed = cfg.walk_speed;   break;
                case PlayerMovementState::Running:   speed = cfg.run_speed;   break;
                case PlayerMovementState::Crouching: speed = cfg.crouch_speed; break;
                case PlayerMovementState::Flying:    speed = cfg.fly_speed;    break;
                default:                             speed = 0.0f;             break;
            }

            vel.value.x = intent.wish_dir.x * speed;
            vel.value.z = intent.wish_dir.z * speed;

            if (ps.current == PlayerMovementState::Flying) {
                vel.value.y = intent.wish_dir.y * cfg.fly_speed;
            }

            // --- Jumping ---
            if (cfg.can_jump && intent.jump && col.is_on_ground && !cfg.can_fly) {
                vel.value.y = std::sqrt(2.0f * GRAVITY * cfg.jump_height);
                ps.current = PlayerMovementState::Jumping;
                col.is_on_ground = false;
            }

            // --- Gravity ---
            if (!col.is_on_ground && !cfg.can_fly) {
                vel.value.y -= GRAVITY * dt;
            }

            // --- State transitions ---
            if (ps.current == PlayerMovementState::Jumping && vel.value.y <= 0.0f) {
                ps.current = PlayerMovementState::Falling;
            }

            if (ps.current == PlayerMovementState::Falling && col.is_on_ground) {
                ps.current = (glm::length2(intent.wish_dir) > 0.01f && cfg.can_walk)
                    ? PlayerMovementState::Walking
                    : PlayerMovementState::Idle;
            }

            // --- Early out if stationary ---
            if (glm::all(glm::epsilonEqual(vel.value, glm::vec3(0.0f), 1e-4f))) {
                col.is_on_ground = isCollidingAt(trans.pos - glm::vec3(0.0f, 0.05f, 0.0f), col, chunkManager);
                col.aabb = col.getBoundingBoxAt(trans.pos);
                return;
            }

            const glm::vec3 oldPos = trans.pos;
            glm::vec3 newPos = trans.pos + vel.value * dt;
            col.is_on_ground = false;

            // --- Axis-by-axis sweep (Y → X → Z) ---
            // Y-axis
            glm::vec3 testPos = { oldPos.x, newPos.y, oldPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.y = testPos.y;
            } else {
                trans.pos.y = oldPos.y;
                if (vel.value.y < 0.0f) {
                    vel.value.y = 0.0f;
                    col.is_on_ground = true;
                } else if (vel.value.y > 0.0f) {
                    vel.value.y = 0.0f;
                }
            }

            // X-axis
            testPos = { newPos.x, trans.pos.y, oldPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.x = testPos.x;
            } else {
                trans.pos.x = oldPos.x;
                vel.value.x = 0.0f;
            }

            // Z-axis
            testPos = { trans.pos.x, trans.pos.y, newPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.z = testPos.z;
            } else {
                trans.pos.z = oldPos.z;
                vel.value.z = 0.0f;
            }

            // --- Final ground check (small downward ray) ---
            if (!col.is_on_ground) {
                col.is_on_ground = isCollidingAt(trans.pos - glm::vec3(0.0f, 0.05f, 0.0f), col, chunkManager);
            }

            // Sync AABB once at the end
            col.aabb = col.getBoundingBoxAt(trans.pos);
        });
}
