#pragma once
#include <memory>
#include <unordered_map>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/glm.hpp>
#include <unordered_set>
#include <vector>
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

struct ivec3_hash {
	std::size_t operator()(const glm::ivec3& v) const noexcept
	{
		std::size_t x = static_cast<std::size_t>(v.x) + 0x80000000;
        std::size_t y = static_cast<std::size_t>(v.y) + 0x80000000;
        std::size_t z = static_cast<std::size_t>(v.z) + 0x80000000;

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
	~ChunkManager() noexcept;

	Shader& getShader() noexcept { return shader; }
	const Shader& getShader() const noexcept { return shader; }

	Shader& getWaterShader() noexcept { return waterShader; }
	const Shader& getWaterShader() const noexcept { return waterShader; }

	const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ivec3_hash>& get_all() const noexcept { return chunks; }

	// Call this every fram to build visibility lists to check which chunks are visible from a given FrustumVolume
	void build_lists(const Transform& ts, const FrustumVolume& fv) noexcept 
	{
		std::vector<DrawArraysIndirectCommand> commands;
		opaqueChunks.clear();
		transparentChunks.clear();
		model_data.clear();

		for (const auto& [pos, chunk_ptr] : chunks) {
			Chunk* c = chunk_ptr.get();
			if (c->visible_face_count > 0 && isAABBInsideFrustum(c->getAABB(), fv)) {
				//if (chunk->isTransparent())
				//transparentChunks.emplace_back(chunk.get());
				//else
				opaqueChunks.emplace_back(c);

				commands.push_back({
						.count = c->visible_face_count * 6,
						.instanceCount = 1,
						.first = (GLuint)(c->faces_offset / sizeof(face_gpu)) * 6,  // vertex offset in faces buffer
						.baseInstance = (GLuint)commands.size()
						});
				model_data.emplace_back(c->getModelMatrix());
			}
		}

		std::size_t needed_model = model_data.size() * sizeof(glm::mat4);
		if (needed_model > model_ssbo_capacity) {
			model_ssbo.resize_grow(needed_model * 2);
			model_ssbo_capacity = model_ssbo.size();
		}
		model_ssbo.update_data(model_data.data(), model_data.size() * sizeof(glm::mat4));
		global_indirect.update_data(commands.data(), commands.size() * sizeof(DrawArraysIndirectCommand));
		if (!transparentChunks.empty()) {
			std::sort(transparentChunks.begin(), transparentChunks.end(),
					[&](const auto& a, const auto& b) {
					float distA = glm::distance(ts.pos, a->getAABB().center());
					float distB = glm::distance(ts.pos, b->getAABB().center());
					return distA > distB; // farthest first
					});
		
		}
	}
	void render_opaque(const Transform& ts, const FrustumVolume& fv) {
		build_lists(ts, fv);  // Rebuild indirect every frame (or only if dirty/camera moved)

		if (opaqueChunks.empty()) return;

		shader.use();
		global_faces.bind_to_slot(7);
		model_ssbo.bind_to_slot(10);
		glBindBuffer(GL_DRAW_INDIRECT_BUFFER, global_indirect.id());

		glMultiDrawArraysIndirect(GL_TRIANGLES, 0, opaqueChunks.size(), 0);  // stride=0 for tight pack
		g_drawCallCount++;
	}

	const std::vector<Chunk*>& get_opaque_chunks() const noexcept { return opaqueChunks; }
	const std::vector<Chunk*>& get_transparent_chunks() const noexcept { return transparentChunks; }

	bool updateBlock(glm::vec3 world_pos, Block::blocks newType) noexcept;
	void generate_chunks(glm::vec3 playerPos, unsigned int renderDistance) noexcept;
	Chunk* getChunk(glm::vec3 world_pos) const noexcept;
	void update_mesh(Chunk *chunk) noexcept;

    private:
	struct MemoryBlock {
		GLintptr offset;
		GLsizeiptr size;
	};
	// A simple list to track holes in the global_faces buffer
	std::vector<MemoryBlock> free_blocks;

	// Helper methods
	GLintptr allocate_faces(GLsizeiptr size);
	void deallocate_faces(GLintptr offset, GLsizeiptr size);

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
	Shader           shader;
	Shader           waterShader;

	// Allocation strategy: simple linear bump allocator for now
	GLintptr MAX_FACES_BUFFER_BYTES = 32 * 1024 * 1024; // 32 MiB → ~5.3M faces (adjust)
	GLsizeiptr model_ssbo_capacity   = 4096 * sizeof(glm::mat4);
	Shader			 voxel_buffer;
	Shader			 write_faces;
	Shader			 add_global_offsets;
	Shader			 prefix_sum;
	Shader			 compute_global_offsets;
	Shader			 indirect;

	SSBO global_faces;               // All visible faces from all chunks
	SSBO global_indirect;            // DrawArraysIndirectCommand per visible chunk
	SSBO			 prefix;
	SSBO			 face_flags;
	SSBO			 group_totals;
	SSBO model_ssbo;
	std::vector<glm::mat4> model_data;
	SSBO temp_faces;
	SSBO temp_indirect;

	std::vector<float> cachedNoiseRegion;
	glm::ivec3         lastRegionSize  = {-1, -1, -1};
	glm::ivec3         lastNoiseOrigin{-999999};

	// TODO: Use an octree to store the chunks
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ivec3_hash> chunks;
	std::vector<Chunk*> opaqueChunks;
	std::vector<Chunk*> transparentChunks;
	std::vector<Chunk*> dirty_chunks;
	std::vector<SSBO> indirect_ssbo_pool;

	// initialize with a value that's != to any reasonable spawn chunk position
	glm::ivec3 last_player_chunk_pos{INT_MIN};

	const static constexpr uint num_workgroups = (Chunk::TOTAL_FACES + 255u) / 256u;
	const static constexpr uint num_scan_groups = (num_workgroups + 255u) / 256u;
	const static constexpr uint flag_words = (Chunk::TOTAL_FACES + 31u) / 32u;

	void update_meshes() noexcept;
	void load_around_pos(glm::ivec3 playerChunkPos, unsigned int renderDistance) noexcept;
	void unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance) noexcept;

};
