#pragma once
#include "core/defines.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/velocity.hpp"
#include "game/ecs/components/collider.hpp"
#include "game/ecs/components/player_state.hpp"
#include "game/ecs/components/movement_config.hpp"
#include "chunk/chunk_manager.hpp"
#include <glm/common.hpp>
#include "core/aabb.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager);

void movement_physics_system(ECS& ecs, ChunkManager& chunkManager, float dt)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<MovementIntent, Velocity, Collider, PlayerState, MovementConfig>(
			[&](Entity e, MovementIntent& intent, Velocity& vel, Collider& col, PlayerState& ps, MovementConfig& cfg) {
			auto* trans = ecs.get_component<Transform>(e);
			if (!trans) return;


			// --- Horizontal movement ---
			float speed = 0.0f;
			switch (ps.current) {
			case PlayerMovementState::Walking:   speed = cfg.walk_speed; break;
			case PlayerMovementState::Running:   speed = cfg.run_speed; break;
			case PlayerMovementState::Crouching: speed = cfg.crouch_speed; break;
			case PlayerMovementState::Flying:    speed = cfg.fly_speed; break;
			default: speed = 0.0f;
			}


			vel.value.x = intent.wish_dir.x * speed;
			vel.value.z = intent.wish_dir.z * speed;
			if (ps.current == PlayerMovementState::Flying) {
				vel.value.y = intent.wish_dir.y * cfg.fly_speed;
			}

			// Decoyple this shit
		    // --- Jumping ---
		    if (cfg.can_jump && intent.jump && col.is_on_ground && !cfg.can_fly) {
			    vel.value.y      = std::sqrt(2 * GRAVITY * cfg.jump_height);
			    ps.current       = PlayerMovementState::Jumping;
			    col.is_on_ground = false; // immediately mark as airborne
		    }

		    // --- Transition Jumping → Falling ---
		    if (ps.current == PlayerMovementState::Jumping && vel.value.y <= 0.0f) {
			    ps.current = PlayerMovementState::Falling;
		    }

		    // --- Transition Falling → Idle/Walking/Running when grounded ---
		    if (ps.current == PlayerMovementState::Falling && col.is_on_ground) {
			    if (glm::length(intent.wish_dir) > 0.1f && cfg.can_walk) {
				    ps.current = PlayerMovementState::Walking;
			    } else {
				    ps.current = PlayerMovementState::Idle;
			    }
		    }


		if(!col.is_on_ground && !cfg.can_fly) vel.value.y -= GRAVITY * dt;

		// Skip early-out only for completely stationary entities
		if (glm::all(glm::epsilonEqual(vel.value, glm::vec3(0.0f), 1e-4f))) {
			col.is_on_ground = isCollidingAt(trans->pos - glm::vec3(0, 0.05f, 0), col, chunkManager);
			// Still sync AABB
			col.aabb = col.getBoundingBoxAt(trans->pos);
			return;
		}

		glm::vec3 oldPos = trans->pos;
		glm::vec3 newPos = trans->pos + vel.value * dt;
		col.is_on_ground = false;

		// --- Axis-by-axis collision resolution ---

		// Y-axis
		glm::vec3 testPos = {oldPos.x, newPos.y, oldPos.z};
		if (!isCollidingAt(testPos, col, chunkManager)) {
			trans->pos.y = testPos.y;
		} else {
			trans->pos.y = oldPos.y;

			if (vel.value.y < 0) {
				// Falling → hit the ground
				vel.value.y     = 0;
				col.is_on_ground = true;
			} else if (vel.value.y > 0) {
				// Jumping → hit ceiling, just stop upward motion
				vel.value.y = 0;
				// do NOT set is_on_ground
			}
		}

		// X-axis
		testPos = {newPos.x, trans->pos.y, oldPos.z};
		if (!isCollidingAt(testPos, col, chunkManager)) {
			trans->pos.x = testPos.x;
		} else {
			trans->pos.x = oldPos.x;
			vel.value.x = 0;
		}

		// Z-axis
		testPos = {trans->pos.x, trans->pos.y, newPos.z};
		if (!isCollidingAt(testPos, col, chunkManager)) {
			trans->pos.z = testPos.z;
		} else {
			trans->pos.z = oldPos.z;
			vel.value.z = 0;
		}

		// Final on-ground check for small epsilon
		if (!col.is_on_ground) {
			col.is_on_ground = isCollidingAt(trans->pos - glm::vec3(0, 0.05f, 0), col, chunkManager);
		}

		// Sync AABB to final position
		col.aabb = col.getBoundingBoxAt(trans->pos);
	});
}

inline bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager)
{
	AABB box = col.getBoundingBoxAt(pos); // let collider provide its bounding box

	glm::ivec3 blockMin = glm::floor(box.min);
	glm::ivec3 blockMax = glm::ceil(box.max) - glm::vec3(1);

	blockMin.y = std::max(blockMin.y, 0);
	blockMax.y = std::clamp(blockMax.y, 0, (int)CHUNK_SIZE.y - 1);

	for (int y = blockMin.y; y <= blockMax.y; ++y) {
		for (int x = blockMin.x; x <= blockMax.x; ++x) {
			for (int z = blockMin.z; z <= blockMax.z; ++z) {
				glm::ivec3 blockWorldPos(x, y, z);
				Chunk*     chunk = chunkManager.getChunk(blockWorldPos);
				if (!chunk)
					return true;

				glm::ivec3   local = blockWorldPos - glm::ivec3(Chunk::chunk_to_world(glm::ivec3(Chunk::world_to_chunk(blockWorldPos))));
				const Block& block = chunk->getBlockAt(local.x, local.y, local.z);

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
