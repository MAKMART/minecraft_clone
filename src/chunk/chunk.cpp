#include "chunk/chunk.hpp"
#include "graphics/renderer/vertex_buffer.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

Chunk::Chunk(const glm::ivec3& pos) : position(pos)
{
	glm::vec3 world = chunk_to_world(position);
	aabb = AABB(world, world + glm::vec3(CHUNK_SIZE));

	DrawArraysIndirectCommand cmd{0, 1, 0, 0};
	const constexpr uint num_workgroups = (TOTAL_FACES + 255u) / 256u;

	faces = SSBO::Dynamic(nullptr, TOTAL_FACES * sizeof(face_gpu));
	indirect_ssbo = SSBO::Dynamic(&cmd, sizeof(DrawArraysIndirectCommand));
	block_ssbo = SSBO::Dynamic(nullptr, sizeof(blocks));
	//normals = SSBO::Dynamic(nullptr, TOTAL_FACES * 4 * sizeof(uint));
}
void Chunk::generate(const FastNoise::SmartNode<FastNoise::FractalFBm>& noise_node, const int SEED) noexcept
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
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
        chunk_noise.data(),
        static_cast<float>(chunk_origin.x),
        static_cast<float>(chunk_origin.y),
        static_cast<float>(chunk_origin.z),
        CHUNK_SIZE.x, CHUNK_SIZE.y, CHUNK_SIZE.z,
        step, step, step,
        SEED
    );
	*/

#define MAX_WORLD_HEIGHT 2
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
				int index = getBlockIndex(x, y, z);
				if (y > height) blocks[index].type = Block::blocks::AIR;
				else if (y == height) blocks[index].type = Block::blocks::GRASS;
				else if (y >= height - dirtDepth) blocks[index].type = Block::blocks::DIRT;
				else blocks[index].type = Block::blocks::STONE;
			}

		}

	block_ssbo.update_data(blocks, sizeof(blocks));
}
void Chunk::render_opaque_mesh(const Shader& shader, GLuint vao) const noexcept
{
	shader.setMat4("model", getModelMatrix());

	faces.bind_to_slot(7);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_ssbo.id());
	glDrawArraysIndirect(GL_TRIANGLES, nullptr);
	g_drawCallCount++;
}
void Chunk::render_transparent_mesh(const Shader& shader) const noexcept
{
	shader.setMat4("model", getModelMatrix());
}
