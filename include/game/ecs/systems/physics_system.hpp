#pragma once
#include "../component_manager.hpp"
#include "../components/position.hpp"
#include "../components/velocity.hpp"
#include "../components/collider.hpp"
#include "../components/player_mode.hpp"
#include "chunk/chunk_manager.hpp"
#include <glm/glm.hpp>
#include "core/defines.hpp"
#include "core/aabb.hpp"

bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager);

template<typename... Components>
void update_physics(ComponentManager<Components...>& cm, ChunkManager& chunkManager, float dt) {
    cm.template for_each_component<Collider>([&](Entity e, Collider& col) {
        auto* pos = cm.template get_component<Position>(e);
        auto* vel = cm.template get_optional_component<Velocity>(e);
        if (!pos || !vel) return;

        // Special-case: player mode behavior
        auto* playerMode = cm.template get_optional_component<PlayerMode>(e);
        if (playerMode && playerMode->mode == Type::SPECTATOR) return;

        // Apply gravity if applicable
        if (!playerMode || playerMode->mode == Type::SURVIVAL) {
            vel->value.y -= GRAVITY;
        }

        // Skip early-out only for completely stationary entities
        if (glm::all(glm::epsilonEqual(vel->value, glm::vec3(0.0f), 1e-4f))) {
            col.is_on_ground = isCollidingAt(pos->value - glm::vec3(0, 0.001f, 0), col, chunkManager);
            // Still sync AABB
            col.aabb = col.getBoundingBoxAt(pos->value);
            return;
        }

        glm::vec3 oldPos = pos->value;
        glm::vec3 newPos = pos->value + vel->value * dt;
        col.is_on_ground = false;

        // --- Axis-by-axis collision resolution ---

        // Y-axis
        glm::vec3 testPos = {oldPos.x, newPos.y, oldPos.z};
        if (!isCollidingAt(testPos, col, chunkManager)) {
            pos->value.y = testPos.y;
        } else {
            pos->value.y = oldPos.y;
            if (vel->value.y < 0) vel->value.y = 0; // stop falling
            col.is_on_ground = true;
        }

        // X-axis
        testPos = {newPos.x, pos->value.y, oldPos.z};
        if (!isCollidingAt(testPos, col, chunkManager)) {
            pos->value.x = testPos.x;
        } else {
            pos->value.x = oldPos.x;
            vel->value.x = 0;
        }

        // Z-axis
        testPos = {pos->value.x, pos->value.y, newPos.z};
        if (!isCollidingAt(testPos, col, chunkManager)) {
            pos->value.z = testPos.z;
        } else {
            pos->value.z = oldPos.z;
            vel->value.z = 0;
        }

        // Final on-ground check for small epsilon
        if (!col.is_on_ground) {
            col.is_on_ground = isCollidingAt(pos->value - glm::vec3(0, 0.001f, 0), col, chunkManager);
        }

        // Sync AABB to final position
        col.aabb = col.getBoundingBoxAt(pos->value);
    });
}


inline bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager) {
    AABB box = col.getBoundingBoxAt(pos); // let collider provide its bounding box

    glm::ivec3 blockMin = glm::floor(box.min);
    glm::ivec3 blockMax = glm::ceil(box.max) - glm::vec3(1);

    blockMin.y = std::max(blockMin.y, 0);
    blockMax.y = std::clamp(blockMax.y, 0, (int)chunkSize.y - 1);

    for (int y = blockMin.y; y <= blockMax.y; ++y) {
        for (int x = blockMin.x; x <= blockMax.x; ++x) {
            for (int z = blockMin.z; z <= blockMax.z; ++z) {
                glm::ivec3 blockWorldPos(x, y, z);
                Chunk* chunk = chunkManager.getChunk(blockWorldPos);
                if (!chunk) return true;

                glm::ivec3 local = blockWorldPos - Chunk::worldToChunk(blockWorldPos) * chunkSize;
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
