#include "chunk/chunk.hpp"
#include <cstdlib>
#include "core/timer.hpp"
#include "graphics/renderer/shader_storage_buffer.hpp"
#include "graphics/shader.hpp"
#include "core/defines.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

Chunk::Chunk(const glm::ivec3& chunkPos)
    : position(chunkPos), logSizeX(std::log2(CHUNK_SIZE.x)), logSizeY(std::log2(CHUNK_SIZE.y))
{

	// Construct AABB
	glm::vec3 worldOrigin = chunkToWorld(position);
	glm::vec3 worldMax    = worldOrigin + glm::vec3(CHUNK_SIZE);
	aabb                  = AABB(worldOrigin, worldMax);
	constexpr uint32_t max_faces = SIZE * 6;

	comp = new Shader("Chunk-compute", SHADERS_DIRECTORY / "chunk.comp");
	face_ssbo = SSBO(nullptr, max_faces * sizeof(face_gpu), SSBO::usage::dynamic_draw);
	counter_ssbo = SSBO(nullptr, sizeof(uint), SSBO::usage::dynamic_draw);
	DrawArraysIndirectCommand cmd{0, 1, 0, 0};
	indirect_ssbo = SSBO(&cmd, sizeof(DrawArraysIndirectCommand), SSBO::usage::dynamic_draw);


	srand(static_cast<unsigned int>(position.x ^ position.y ^ position.z));
}
Chunk::~Chunk()
{
	if (comp) delete comp;
}
void Chunk::generate(std::span<const float> fullNoise, int regionWidth, int noiseOffsetX, int noiseOffsetZ)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	Timer generation_timer("Chunk::generate");


	constexpr int dirtDepth  = 3;
	constexpr int beachDepth = 1; // How wide the beach is vertically

	for (int z = 0; z < CHUNK_SIZE.z; ++z) {
		for (int y = 0; y < CHUNK_SIZE.y; ++y) {
			for (int x = 0; x < CHUNK_SIZE.x; ++x) {
				int noiseIndex = (noiseOffsetZ + z) * regionWidth + (noiseOffsetX + x);
#if defined(DEBUG)
				if (noiseIndex < 0 || noiseIndex >= static_cast<int>(fullNoise.size())) {
					log::system_warn("Chunk", "Noise index out of bounds: {}", noiseIndex);
					continue;
				}
#endif
				int height = static_cast<int>(fullNoise[noiseIndex] * CHUNK_SIZE.y);
				height     = std::clamp(height, 0, CHUNK_SIZE.y - 1);

				int index = getBlockIndex(x, y, z);
#if defined (DEBUG)
				if (index == -1)
					continue;
#endif

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
	block_ssbo = SSBO(packed_blocks, sizeof(packed_blocks), SSBO::usage::dynamic_draw);
}
void Chunk::updateMesh()
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	uint zero = 0;
	counter_ssbo.update_data(&zero, sizeof(uint));

	if (block_ssbo.id())
		block_ssbo.update_data(packed_blocks, sizeof(packed_blocks));

	/*
	size_t max_faces = 6 * SIZE;
	std::vector<face_gpu> zero_faces(max_faces);
	face_ssbo.update_data(zero_faces.data(),
			zero_faces.size() * sizeof(face_gpu));


	DrawArraysIndirectCommand zero_cmd{0, 1, 0, 0};
	indirect_ssbo.update_data(&zero_cmd, sizeof(zero_cmd));

	*/

	block_ssbo.bind_to_slot(1);
	face_ssbo.bind_to_slot(2);
	counter_ssbo.bind_to_slot(3);
	indirect_ssbo.bind_to_slot(4);
	comp->use();
	glDispatchCompute(CHUNK_SIZE.x/8, CHUNK_SIZE.y/8, CHUNK_SIZE.z/8);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_DRAW_INDIRECT_BUFFER | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

}
void Chunk::renderOpaqueMesh(const Shader& shader, GLuint vao)
{
	shader.setMat4("model", getModelMatrix());

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirect_ssbo.id());
	face_ssbo.bind_to_slot(2);

	glDrawArraysIndirect(GL_TRIANGLES, nullptr);
	g_drawCallCount++;
}
void Chunk::renderTransparentMesh(const Shader& shader)
{
	shader.setMat4("model", getModelMatrix());
}
