#pragma once
#include "core/defines.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/velocity.hpp"
#include "game/ecs/components/collider.hpp"
#include "game/ecs/components/player_mode.hpp"
#include "game/ecs/components/player_state.hpp"
#include "chunk/chunk_manager.hpp"
#include <glm/glm.hpp>
#include "core/aabb.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager);

void update_physics(ECS& ecs, ChunkManager& chunkManager, float dt)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_component<Collider>([&](Entity e, Collider& col) {
		auto* trans = ecs.get_component<Transform>(e);
		auto* vel = ecs.get_component<Velocity>(e);
		if (!trans || !vel)
			return;

		// Special-case: player mode behavior
		auto* playerMode  = ecs.get_component<PlayerMode>(e);
		auto* playerState = ecs.get_component<PlayerState>(e);
		if (playerMode && playerMode->mode == Type::SPECTATOR)
			return;

		// Apply gravity if applicable
		if (!playerMode || !playerState || playerState->current != PlayerMovementState::Flying) {
			vel->value.y -= GRAVITY * dt; // Intgrate gravitational acceleration
		}

		// Skip early-out only for completely stationary entities
		if (glm::all(glm::epsilonEqual(vel->value, glm::vec3(0.0f), 1e-4f))) {
			col.is_on_ground = isCollidingAt(trans->pos - glm::vec3(0, 0.001f, 0), col, chunkManager);
			// Still sync AABB
			col.aabb = col.getBoundingBoxAt(trans->pos);
			return;
		}

		glm::vec3 oldPos = trans->pos;
		glm::vec3 newPos = trans->pos + vel->value * dt;
		col.is_on_ground = false;

		// --- Axis-by-axis collision resolution ---

		// Y-axis
		glm::vec3 testPos = {oldPos.x, newPos.y, oldPos.z};
		if (!isCollidingAt(testPos, col, chunkManager)) {
			trans->pos.y = testPos.y;
		} else {
			trans->pos.y = oldPos.y;

			if (vel->value.y < 0) {
				// Falling → hit the ground
				vel->value.y     = 0;
				col.is_on_ground = true;
			} else if (vel->value.y > 0) {
				// Jumping → hit ceiling, just stop upward motion
				vel->value.y = 0;
				// do NOT set is_on_ground
			}
		}

		// X-axis
		testPos = {newPos.x, trans->pos.y, oldPos.z};
		if (!isCollidingAt(testPos, col, chunkManager)) {
			trans->pos.x = testPos.x;
		} else {
			trans->pos.x = oldPos.x;
			vel->value.x = 0;
		}

		// Z-axis
		testPos = {trans->pos.x, trans->pos.y, newPos.z};
		if (!isCollidingAt(testPos, col, chunkManager)) {
			trans->pos.z = testPos.z;
		} else {
			trans->pos.z = oldPos.z;
			vel->value.z = 0;
		}

		// Final on-ground check for small epsilon
		if (!col.is_on_ground) {
			col.is_on_ground = isCollidingAt(trans->pos - glm::vec3(0, 0.001f, 0), col, chunkManager);
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
	blockMax.y = std::clamp(blockMax.y, 0, (int)chunkSize.y - 1);

	for (int y = blockMin.y; y <= blockMax.y; ++y) {
		for (int x = blockMin.x; x <= blockMax.x; ++x) {
			for (int z = blockMin.z; z <= blockMax.z; ++z) {
				glm::ivec3 blockWorldPos(x, y, z);
				Chunk*     chunk = chunkManager.getChunk(blockWorldPos);
				if (!chunk)
					return true;

				glm::ivec3   local = blockWorldPos - Chunk::worldToChunk(blockWorldPos) * chunkSize;
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
