module;
// #include <memory>
// #include <unordered_map>
// #include <vector>
#include <glad/glad.h>
#include <climits>
export module chunk_manager;

import std;
import core;
import glm;
import ecs_components;
export import chunk;
import noise;
import ssbo;
import aabb;
import chunk_renderer;
import shader;
import mesher;
import noise_2;
import utility;

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
		ChunkManager();
		~ChunkManager() noexcept { 

			chunks.clear(); 
			delete[] mainThreadMeshData.opaqueMask;
			delete[] mainThreadMeshData.faceMasks;
			delete[] mainThreadMeshData.forwardMerged;
			delete[] mainThreadMeshData.rightMerged;
			delete mainThreadMeshData.vertices;

		}

		Shader& getShader2() noexcept { return shader2; }
		const Shader& getShader2() const noexcept { return shader2; }

		Shader& getShader() noexcept { return shader; }
		const Shader& getShader() const noexcept { return shader; }

		Shader& getWaterShader() noexcept { return waterShader; }
		const Shader& getWaterShader() const noexcept { return waterShader; }

		const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ivec3_hash>& get_all() const noexcept { return chunks; }

		// Call this every fram to build visibility lists to check which chunks are visible from a given FrustumVolume
		void build_lists(const Transform& ts, const FrustumVolume& fv) noexcept;
		void render_opaque(const Transform& ts, const FrustumVolume& fv) noexcept;

		const std::vector<Chunk*>& get_opaque_chunks() const noexcept { return opaqueChunks; }
		const std::vector<Chunk*>& get_transparent_chunks() const noexcept { return transparentChunks; }

		bool update_block(glm::ivec3 world_pos, Block::blocks newType) noexcept;
		void generate_chunks(glm::vec3 playerPos, unsigned int renderDistance) noexcept;
		Chunk* getChunk(glm::ivec3 world_pos) const noexcept;
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
		Noise			 noise_2;
		Shader           shader;
		Shader			 shader2;
		Shader           waterShader;

		// Allocation strategy: simple linear bump allocator for now
		GLintptr MAX_FACES_BUFFER_BYTES = 32 * 1024 * 1024; // 32 MiB → ~5.3M faces (adjust)
		GLsizeiptr offset_ssbo_capacity = 4096 * sizeof(glm::vec4);
		Shader			 voxel_buffer;
		Shader			 write_faces;
		Shader			 merged_offsets;
		Shader			 prefix_sum;
		Shader			 indirect;
    

		dynamic_ssbo			 global_faces;
		dynamic_ssbo			 global_indirect;
		persistent_ssbo			 prefix;
		persistent_ssbo face_flags;
		persistent_ssbo group_totals;
		dynamic_ssbo offset_ssbo;
		persistent_ssbo temp_faces;
		// ssbo			 temp_indirect;

		std::vector<glm::vec4> offset_data;

		std::vector<float> cachedNoiseRegion;
		glm::ivec3         lastRegionSize  = {-1, -1, -1};
		glm::ivec3         lastNoiseOrigin{-999999};

		// TODO: Use an octree to store the chunks
		std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>, ivec3_hash> chunks;

		struct ChunkRenderData {
			glm::ivec3 chunkPos = glm::ivec3(0);
			std::vector<DrawElementsIndirectCommand> faceDrawCommands = std::vector<DrawElementsIndirectCommand>(6);
		};

		std::vector<ChunkRenderData> chunkRenderData;
		MeshData mainThreadMeshData;
		ChunkRenderer chunkRenderer;

		std::vector<Chunk*> opaqueChunks;
		std::vector<Chunk*> transparentChunks;
		std::vector<Chunk*> dirty_chunks;

		// initialize with a value that's != to any reasonable spawn chunk position
		glm::ivec3 last_player_chunk_pos{INT_MIN};

		const static constexpr unsigned int num_workgroups = (TOTAL_FACES + 255u) / 256u;
		const static constexpr unsigned int num_scan_groups = (num_workgroups + 255u) / 256u;
		const static constexpr unsigned int flag_words = (TOTAL_FACES + 31u) / 32u;

		void update_meshes() noexcept;
		void load_around_pos(glm::ivec3 playerChunkPos, unsigned int renderDistance) noexcept;
		void unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance) noexcept;

};
