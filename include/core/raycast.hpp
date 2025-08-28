#pragma once
#include <glm/glm.hpp>
#include <optional>
#include "chunk/chunk_manager.hpp"

class raycast
{
      public:
	// Return world-space voxel position
	static std::optional<glm::ivec3> voxel(ChunkManager& chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance)
	{
		// Initialize voxel position (world-space)
		glm::ivec3 worldResult(
		    static_cast<int>(std::floor(rayOrigin.x)),
		    static_cast<int>(std::floor(rayOrigin.y)),
		    static_cast<int>(std::floor(rayOrigin.z)));

		// Step direction for each axis
		glm::ivec3 step(
		    (rayDirection.x >= 0) ? 1 : -1,
		    (rayDirection.y >= 0) ? 1 : -1,
		    (rayDirection.z >= 0) ? 1 : -1);

		// Compute tMax: distance until first voxel boundary is crossed
		glm::vec3 tMax(
		    (rayDirection.x != 0.0f) ? ((step.x > 0 ? (worldResult.x + 1.0f - rayOrigin.x) : (rayOrigin.x - worldResult.x)) / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
		    (rayDirection.y != 0.0f) ? ((step.y > 0 ? (worldResult.y + 1.0f - rayOrigin.y) : (rayOrigin.y - worldResult.y)) / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
		    (rayDirection.z != 0.0f) ? ((step.z > 0 ? (worldResult.z + 1.0f - rayOrigin.z) : (rayOrigin.z - worldResult.z)) / std::abs(rayDirection.z)) : std::numeric_limits<float>::max());

		// Compute tDelta: how far to move in t to cross a voxel
		glm::vec3 tDelta(
		    (rayDirection.x != 0.0f) ? (1.0f / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
		    (rayDirection.y != 0.0f) ? (1.0f / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
		    (rayDirection.z != 0.0f) ? (1.0f / std::abs(rayDirection.z)) : std::numeric_limits<float>::max());

		float t = 0.0f;
		while (t <= maxDistance) {
			Chunk* chunk = chunkManager.getChunk(worldResult);

			if (chunk) {
				glm::ivec3 localVoxelPos = Chunk::worldToLocal(worldResult);
				int        blockIndex    = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

				if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
				    chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
					return worldResult; // Found a solid block
				}
			}

			if (tMax.x < tMax.y) {
				if (tMax.x < tMax.z) {
					worldResult.x += step.x;
					t = tMax.x;
					tMax.x += tDelta.x;
				} else {
					worldResult.z += step.z;
					t = tMax.z;
					tMax.z += tDelta.z;
				}
			} else {
				if (tMax.y < tMax.z) {
					worldResult.y += step.y;
					t = tMax.y;
					tMax.y += tDelta.y;
				} else {
					worldResult.z += step.z;
					t = tMax.z;
					tMax.z += tDelta.z;
				}
			}
		}

		return std::nullopt;
	}

	// Return world-space voxel position
	static std::optional<std::pair<glm::ivec3, glm::ivec3>> voxel_normals(
	    ChunkManager& chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance)
	{
		glm::ivec3 worldResult = glm::floor(rayOrigin);
		glm::ivec3 step        = glm::sign(rayDirection); // Step direction (-1 or +1)

		glm::vec3 tMax;
		glm::vec3 tDelta = glm::vec3(
		    (rayDirection.x != 0.0f) ? (1.0f / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
		    (rayDirection.y != 0.0f) ? (1.0f / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
		    (rayDirection.z != 0.0f) ? (1.0f / std::abs(rayDirection.z)) : std::numeric_limits<float>::max());

		for (int i = 0; i < 3; i++) {
			if (rayDirection[i] > 0)
				tMax[i] = (worldResult[i] + 1 - rayOrigin[i]) * tDelta[i];
			else
				tMax[i] = (rayOrigin[i] - worldResult[i]) * tDelta[i];
		}

		float      t         = 0.0f;
		glm::ivec3 lastVoxel = worldResult;

		while (t < maxDistance) {
			Chunk* chunk = chunkManager.getChunk({worldResult.x, 0, worldResult.z});

			if (chunk) {
				glm::ivec3 localVoxelPos = Chunk::worldToLocal(worldResult);
				int        blockIndex    = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

				if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
				    chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
					return std::make_pair(worldResult, worldResult - lastVoxel);
				}
			}

			lastVoxel = worldResult;

			if (tMax.x < tMax.y) {
				if (tMax.x < tMax.z) {
					worldResult.x += step.x;
					t = tMax.x;
					tMax.x += tDelta.x;
				} else {
					worldResult.z += step.z;
					t = tMax.z;
					tMax.z += tDelta.z;
				}
			} else {
				if (tMax.y < tMax.z) {
					worldResult.y += step.y;
					t = tMax.y;
					tMax.y += tDelta.y;
				} else {
					worldResult.z += step.z;
					t = tMax.z;
					tMax.z += tDelta.z;
				}
			}
		}

		return std::nullopt;
	}
};
