#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include "graphics/renderer/vertex_buffer.hpp"
#include "graphics/shader.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

Chunk::Chunk(const glm::ivec3& pos) : position(pos)
{
	glm::vec3 world = chunk_to_world(position);
	aabb = AABB(world, world + glm::vec3(CHUNK_SIZE));

	DrawArraysIndirectCommand cmd{0, 1, 0, 0};
	const constexpr uint num_workgroups = (TOTAL_FACES + 255u) / 256u;

	indirect_ssbo = SSBO::Dynamic(&cmd, sizeof(DrawArraysIndirectCommand));
	block_ssbo = SSBO::Dynamic(nullptr, sizeof(block_types));
	//normals = SSBO::Dynamic(nullptr, TOTAL_FACES * 4 * sizeof(uint));

	model = glm::translate(glm::mat4(1.0f), chunk_to_world(position));
}
void Chunk::generate(const FastNoise::SmartNode<FastNoise::FractalFBm>& noise_node, const int SEED) noexcept
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	non_air_count = 0;
	constexpr int dirtDepth  = 3;
	constexpr int beachDepth = 1; // How wide the beach is vertically

	glm::vec3 chunk_origin = Chunk::chunk_to_world(position);

    constexpr float step = 1.0f; // 1:1 per voxel, adjust if you want smoother noise

	noise_node->GenUniformGrid2D(
			chunk_noise,
			static_cast<float>(chunk_origin.x),
			static_cast<float>(chunk_origin.z),
			CHUNK_SIZE.x, CHUNK_SIZE.z,
			step,
			step,
			SEED
			);
	/*
    noise_node->GenUniformGrid3D(
        chunk_noise,
        static_cast<float>(chunk_origin.x),
        static_cast<float>(chunk_origin.y),
        static_cast<float>(chunk_origin.z),
        CHUNK_SIZE.x, CHUNK_SIZE.y, CHUNK_SIZE.z,
        step, step, step,
        SEED
    );
	*/

#define MAX_WORLD_HEIGHT 0
	int noise_index = 0;
	for (int x = 0; x < CHUNK_SIZE.x; ++x)
		for (int z = 0; z < CHUNK_SIZE.z; ++z)
		{
			int x_world = chunk_origin.x + x;
			int z_world = chunk_origin.z + z;
			float n = chunk_noise[noise_index++];
			int world_height = static_cast<int>(n * MAX_WORLD_HEIGHT);
			int height = world_height - chunk_origin.y;

			for (int y = 0; y < CHUNK_SIZE.y; ++y)
			{
				int index = get_index(x, y, z);
				Block::blocks type;
				if (y > height)
					type = Block::blocks::AIR;
				else if (y == height)
					type = Block::blocks::GRASS;
				else if (y >= height - dirtDepth)
					type = Block::blocks::DIRT;
				else
					type = Block::blocks::STONE;
				block_types[index] = static_cast<std::uint8_t>(type);

				if (type != Block::blocks::AIR)
					++non_air_count;
			}

		}

	if (has_any_blocks()) {
		faces = SSBO::Dynamic(nullptr, TOTAL_FACES * sizeof(face_gpu));
		block_ssbo.update_data(block_types, sizeof(block_types));
	} else {
		faces = SSBO::Dynamic(nullptr, 1);
	}
}
void Chunk::render_opaque_mesh(const Shader& shader, GLuint vao) const noexcept
{
	if (!has_any_blocks())
		return;
	
	shader.setMat4("model", model);

	faces.bind_to_slot(7);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_ssbo.id());
	glDrawArraysIndirect(GL_TRIANGLES, nullptr);
	g_drawCallCount++;
}
void Chunk::render_transparent_mesh(const Shader& shader) const noexcept
{
	shader.setMat4("model", model);
}
