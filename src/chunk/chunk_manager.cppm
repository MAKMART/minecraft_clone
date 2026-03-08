module;
#include <memory>
#include <unordered_map>
#include <vector>
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "core/timer.hpp"

export module chunk_manager;

import core;
import glm;
import ecs_components;
export import chunk;
import noise;
import logger;
import ssbo;
import aabb;

export struct ivec3_hash {
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

export class ChunkManager
{
      public:
		  ChunkManager()
			  : noise(92368123),
			  shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
			  waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY),
			  voxel_buffer("voxel_buffer", SHADERS_DIRECTORY / "voxel_buffer.comp"),
			  write_faces("write_faces", SHADERS_DIRECTORY / "write_faces.comp"),
			  merged_offsets("merged_offsets", SHADERS_DIRECTORY / "merged_offsets.comp"),
			  prefix_sum("prefix_sum", SHADERS_DIRECTORY / "prefix_sum.comp"),
			  indirect("indirect", SHADERS_DIRECTORY / "indirect_buffer_fill.comp"),
			  // Could be PersistentWrite
			  global_faces(SSBO::Dynamic(nullptr, MAX_FACES_BUFFER_BYTES)),  // or just Dynamic
			  global_indirect(SSBO::Dynamic(nullptr, 4096 * sizeof(DrawArraysIndirectCommand))),  // generous initial size
			  prefix(SSBO::PersistentWrite(nullptr, Chunk::TOTAL_FACES * sizeof(std::uint8_t))),
			  face_flags(SSBO::PersistentWrite(nullptr, flag_words * sizeof(std::uint8_t))),
			  group_totals(SSBO::PersistentWrite(nullptr, num_workgroups * sizeof(std::uint8_t))),
			  model_ssbo(SSBO::Dynamic(nullptr, model_ssbo_capacity)),
			  temp_faces(SSBO::PersistentWrite(nullptr, Chunk::TOTAL_FACES * sizeof(face_gpu)))
	{

		if constexpr (CHUNK_SIZE.x <= 0 || CHUNK_SIZE.y <= 0 || CHUNK_SIZE.z <= 0) {
			log::system_error("ChunkManager", "CHUNK_SIZE < 0");
			std::exit(1);
		}
		if constexpr ((CHUNK_SIZE.x & (CHUNK_SIZE.x - 1)) != 0 || (CHUNK_SIZE.y & (CHUNK_SIZE.y - 1)) != 0 || (CHUNK_SIZE.z & (CHUNK_SIZE.z - 1)) != 0) {
			log::system_error("ChunkManager", "CHUNK_SIZE must be a power of 2");
			std::exit(1);
		}

		voxel_buffer.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
		prefix_sum.setUInt("total_faces", Chunk::TOTAL_FACES);
		write_faces.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
		write_faces.setUInt("total_faces", Chunk::TOTAL_FACES);
		shader.setUVec3("CHUNK_SIZE", CHUNK_SIZE);
		merged_offsets.setUInt("num_groups", num_workgroups);
		merged_offsets.setUInt("total_faces", Chunk::TOTAL_FACES);

		free_blocks.emplace_back(0, MAX_FACES_BUFFER_BYTES);
		opaqueChunks.reserve(chunks.size());
		transparentChunks.reserve(chunks.size());
		DrawArraysIndirectCommand cmd{0, 1, 0, 0};
		temp_indirect = SSBO::PersistentWrite(&cmd, sizeof(DrawArraysIndirectCommand));
	}
	~ChunkManager() noexcept { chunks.clear(); }

	Shader& getShader() noexcept { return shader; }
	const Shader& getShader() const noexcept { return shader; }
			
	Shader& getWaterShader() noexcept { return waterShader; }
	const Shader& getWaterShader() const noexcept { return waterShader; }

	const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ivec3_hash>& get_all() const noexcept { return chunks; }

	// Call this every fram to build visibility lists to check which chunks are visible from a given FrustumVolume
	void build_lists(const Transform& ts, const FrustumVolume& fv) noexcept;
	void render_opaque(const Transform& ts, const FrustumVolume& fv) noexcept {
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

	bool updateBlock(glm::vec3 world_pos, Block::blocks newType) noexcept
	{
		Chunk* chunk = getChunk(world_pos);
		if (!chunk)
			return false;
		glm::ivec3 localPos = Chunk::world_to_local(world_pos);

		chunk->set_block_type(localPos.x, localPos.y, localPos.z, newType);
		if (!chunk->in_dirty_list) {
			dirty_chunks.emplace_back(chunk);
			chunk->in_dirty_list = true;
		}
		return true;
	}
	void generate_chunks(glm::vec3 playerPos, unsigned int renderDistance) noexcept
	{
#if defined(TRACY_ENABLE)
		ZoneScoped;
#endif
		glm::ivec3 playerChunk{Chunk::world_to_chunk(playerPos)};

		if (playerChunk != last_player_chunk_pos) {
			load_around_pos(playerChunk, renderDistance);
			unload_around_pos(playerChunk, renderDistance + 1);
			last_player_chunk_pos = playerChunk;
		}
		update_meshes();
	}
	Chunk* getChunk(glm::vec3 world_pos) const noexcept
	{
#if defined(TRACY_ENABLE)
		ZoneScoped;
#endif
		auto it = chunks.find(Chunk::world_to_chunk(world_pos));
		if (it != chunks.end())
			return it->second.get();
		else {
#if defined(DEBUG)
			log::system_error("ChunkManager", "Chunk at glm::vec3({}, {}, {}) not found!", world_pos.x, world_pos.y, world_pos.z);
#endif
			return nullptr;
		}
	}
	void update_mesh(Chunk *chunk) noexcept;

	const NoiseSystem& get_noise() const noexcept { return noise; }

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
	static bool isAABBInsideFrustum(const AABB& aabb, const FrustumVolume& fv) noexcept {
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

	NoiseSystem		 noise;
	Shader           shader;
	Shader           waterShader;

	// Allocation strategy: simple linear bump allocator for now
	GLintptr MAX_FACES_BUFFER_BYTES = 32 * 1024 * 1024; // 32 MiB → ~5.3M faces (adjust)
	GLsizeiptr model_ssbo_capacity   = 4096 * sizeof(glm::mat4);
	Shader			 voxel_buffer;
	Shader			 write_faces;
	Shader			 merged_offsets;
	Shader			 prefix_sum;
	Shader			 indirect;

	SSBO			 global_faces;
	SSBO			 global_indirect;
	SSBO			 prefix;
	SSBO			 face_flags;
	SSBO			 group_totals;
	SSBO			 model_ssbo;
	SSBO			 temp_faces;
	SSBO			 temp_indirect;

	std::vector<glm::mat4> model_data;

	std::vector<float> cachedNoiseRegion;
	glm::ivec3         lastRegionSize  = {-1, -1, -1};
	glm::ivec3         lastNoiseOrigin{-999999};

	// TODO: Use an octree to store the chunks
	std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ivec3_hash> chunks;
	std::vector<Chunk*> opaqueChunks;
	std::vector<Chunk*> transparentChunks;
	std::vector<Chunk*> dirty_chunks;

	// initialize with a value that's != to any reasonable spawn chunk position
	glm::ivec3 last_player_chunk_pos{INT_MIN};

	const static constexpr uint num_workgroups = (Chunk::TOTAL_FACES + 255u) / 256u;
	const static constexpr uint num_scan_groups = (num_workgroups + 255u) / 256u;
	const static constexpr uint flag_words = (Chunk::TOTAL_FACES + 31u) / 32u;

	void update_meshes() noexcept
	{
		if (dirty_chunks.empty()) return;

		face_flags.bind_to_slot(2);
		prefix.bind_to_slot(5);
		group_totals.bind_to_slot(6);
		temp_faces.bind_to_slot(7);
		for (auto& chunk : dirty_chunks) {
			update_mesh(chunk);
			chunk->in_dirty_list = false;
			chunk->changed = false;
		}
		dirty_chunks.clear();
	}
	void load_around_pos(glm::ivec3 playerChunkPos, unsigned int renderDistance) noexcept;
	void unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance) noexcept;

};
