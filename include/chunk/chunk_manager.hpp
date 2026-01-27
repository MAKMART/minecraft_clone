#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <vector>
#include <tuple>
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include <FastNoise/FastNoise.h>
#include <optional>
#include "glm/ext/vector_int3.hpp"
#include "core/logger.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include <functional>
#include <future>

struct ivec3_hash {
	std::size_t operator()(const glm::ivec3& v) const noexcept
	{
		std::size_t x = static_cast<std::size_t>(v.x);
		std::size_t y = static_cast<std::size_t>(v.y);
		std::size_t z = static_cast<std::size_t>(v.z);

		const std::size_t prime1 = 73856093;
		const std::size_t prime2 = 19349663;
		const std::size_t prime3 = 83492791;

		return (x * prime1) ^ (y * prime2) ^ (z * prime3);
	}
};

class ChunkManager
{
      public:
	ChunkManager(std::optional<int> seed = std::nullopt);
	~ChunkManager();

	Shader& getShader() { return shader; }

	Shader& getWaterShader(){ return waterShader; }

	Shader& getCompute() { return compute; }

	const Shader& getShader() const { return shader; }

	const Shader& getWaterShader() const { return waterShader; }

	const Shader& getCompute() const { return compute; }

	const GLuint& getVAO() const { return VAO; }

	const std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, ivec3_hash>& getChunks() const { return chunks; }

	std::vector<Chunk*> getOpaqueChunks(const FrustumVolume& fv) const
	{
		std::vector<Chunk*> opaqueChunks;
		for (const auto& chunk : chunks) {
		if (isAABBInsideFrustum(chunk.second->getAABB(), fv)) {
			opaqueChunks.emplace_back(chunk.second.get());
		}
		}
		return opaqueChunks;
	}

	std::vector<Chunk*> getTransparentChunks(const FrustumVolume& fv, const Transform& ts) const
	{
		std::vector<Chunk*> transparentChunks;
		for (const auto& chunk : chunks) {
			if (isAABBInsideFrustum(chunk.second->getAABB(), fv)) {
				transparentChunks.emplace_back(chunk.second.get());
			}
		}
		std::sort(transparentChunks.begin(), transparentChunks.end(),
				[&](const auto& a, const auto& b) {
				float distA = glm::distance(ts.pos, a->getAABB().center());
				float distB = glm::distance(ts.pos, b->getAABB().center());
				return distA > distB; // farthest first
				});
		return transparentChunks;
	}

	bool updateBlock(glm::vec3 world_pos, Block::blocks newType);

	void updateChunk(glm::vec3 world_pos) noexcept
	{
		if (auto chunk = getChunk(world_pos))
			chunk->updateMesh();
		else
			return;
	}

	void generateChunks(glm::vec3 playerPos, unsigned int renderDistance);

	Chunk* getChunk(glm::vec3 worldPos) const;

	bool getorCreateChunk(glm::vec3 worldPos);

	void unloadChunk(glm::vec3 world_pos);

	void clearChunks() noexcept
	{
		chunks.clear();
	}

    private:

	//TODO: Maybe move this into a utils namespace or smth
	static bool isAABBInsideFrustum(const AABB& aabb, const FrustumVolume& fv) {
		for (const auto& plane : fv.planes) { // assuming you store planes publicly or via accessor
			glm::vec3 normal = plane.normal();

			// Get the positive vertex along the plane normal
			glm::vec3 p = aabb.min;
			if (normal.x >= 0) p.x = aabb.max.x;
			if (normal.y >= 0) p.y = aabb.max.y;
			if (normal.z >= 0) p.z = aabb.max.z;

			// Distance from plane to positive vertex
			float distance = glm::dot(normal, p) + plane.offset();
			if (distance < 0)
				return false; // AABB is completely outside this plane
		}
		return true; // Intersects or is fully inside
	}


	FastNoise::SmartNode<FastNoise::Perlin> perlin_node;
	FastNoise::SmartNode<FastNoise::FractalFBm> fractal_node;
	static constexpr int SEED = 1337;
	GLuint           VAO;
	Shader           shader;
	Shader           waterShader;
	Shader			 compute;

	std::vector<float> cachedNoiseRegion;
	glm::ivec2         lastRegionSize  = {-1, -1};
	glm::ivec3         lastNoiseOrigin = {-999999, 0, -999999};

	// TODO: Use an octree to store the chunks
	std::unordered_map<glm::ivec3, std::shared_ptr<Chunk>, ivec3_hash> chunks;

	// initialize with a value that's != to any reasonable spawn chunk position
	glm::ivec3 last_player_chunk_pos{INT_MIN};

	std::shared_ptr<Chunk> getOrCreateChunk(glm::vec3 worldPos);

	void loadChunksAroundPos(const glm::ivec3& playerChunkPos, int renderDistance);

	void unloadDistantChunks(const glm::ivec3& playerChunkPos, int unloadDistance);

};
