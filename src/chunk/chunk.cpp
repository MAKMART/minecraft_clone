#include "chunk/chunk.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

Chunk::Chunk(const glm::ivec3& pos)
    : position(pos)
{
	glm::vec3 world = chunk_to_world(position);
	aabb = AABB(world, world + glm::vec3(CHUNK_SIZE));

	// Fill face_ssbo with max possible faces (SIZE * 6)
	face_ssbo = SSBO(nullptr, SIZE * 6 * sizeof(face_gpu), SSBO::usage::dynamic_draw);
	counter_ssbo = SSBO(nullptr, sizeof(uint), SSBO::usage::dynamic_draw);
	DrawArraysIndirectCommand cmd{0, 1, 0, 0};
	indirect_ssbo = SSBO(&cmd, sizeof(DrawArraysIndirectCommand), SSBO::usage::dynamic_draw);
	block_ssbo = SSBO(nullptr, sizeof(packed_blocks), SSBO::usage::dynamic_draw);

	srand(static_cast<unsigned int>(position.x ^ position.y ^ position.z));
}
void Chunk::generate(std::span<const float> fullNoise, int regionWidth, int noiseOffsetX, int noiseOffsetZ)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	constexpr int dirtDepth  = 3;
	constexpr int beachDepth = 1; // How wide the beach is vertically

	for (int z = 0; z < CHUNK_SIZE.z; ++z) {
		for (int x = 0; x < CHUNK_SIZE.x; ++x) {
			int noiseIndex = (noiseOffsetZ + z) * regionWidth + (noiseOffsetX + x);
			int height = static_cast<int>(fullNoise[noiseIndex] * CHUNK_SIZE.y);
			height     = std::clamp(height, 0, CHUNK_SIZE.y - 1);
			for (int y = 0; y < CHUNK_SIZE.y; ++y) {
				int index = getBlockIndex(x, y, z);
				Block::blocks block_type;
				if (y > height) {
					block_type = Block::blocks::AIR;
				} else if (y == height) {
					block_type = (height <= seaLevel + beachDepth) ? Block::blocks::SAND : Block::blocks::GRASS;
				} else if (y >= height - dirtDepth) {
					block_type = (height <= seaLevel + beachDepth) ? Block::blocks::SAND : Block::blocks::DIRT;
				} else {
					block_type = Block::blocks::STONE;
				}
				blocks[index].type = block_type;
				uint32_t b = static_cast<uint8_t>(block_type);
				packed_blocks[index / 4] |= b << ((index % 4) * 8);
			}
		}
	}
	block_ssbo.update_data(packed_blocks, sizeof(packed_blocks));
}
void Chunk::updateMesh()
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	uint zero = 0;
	counter_ssbo.update_data(&zero, sizeof(uint));

	if (block_ssbo.id() && dirty)
		block_ssbo.update_data(packed_blocks, sizeof(packed_blocks));

	block_ssbo.bind_to_slot(1);
	face_ssbo.bind_to_slot(2);
	counter_ssbo.bind_to_slot(3);
	indirect_ssbo.bind_to_slot(4);
	glDispatchCompute(CHUNK_SIZE.x/8, CHUNK_SIZE.y/8, CHUNK_SIZE.z/4);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_DRAW_INDIRECT_BUFFER);
	dirty = false;
}
void Chunk::renderOpaqueMesh(const Shader& shader, GLuint vao) const noexcept
{
	shader.setMat4("model", getModelMatrix());

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_ssbo.id());
	face_ssbo.bind_to_slot(2);
	glDrawArraysIndirect(GL_TRIANGLES, nullptr);
	g_drawCallCount++;
}
void Chunk::renderTransparentMesh(const Shader& shader) const noexcept
{
	shader.setMat4("model", getModelMatrix());
}
