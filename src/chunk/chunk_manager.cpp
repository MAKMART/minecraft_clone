#include "chunk/chunk_manager.hpp"
#include <cmath>
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include "core/timer.hpp"
#include <glm/gtx/norm.hpp>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

ChunkManager::ChunkManager(std::optional<int> seed)
    : shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
      waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY),
	  compute("Chunk_compute", SHADERS_DIRECTORY / "chunk.comp"),
	  reset("Chunk_reset", SHADERS_DIRECTORY / "buf_reset.comp"),
	  indirect("Chunk_indirect", SHADERS_DIRECTORY / "indirect_buffer_fill.comp")
{

	log::info("Loading texture atlas from {} with working dir = {}", BLOCK_ATLAS_TEXTURE_DIRECTORY.string(), WORKING_DIRECTORY.string());
	log::info("Initializing Chunk Manager...");

	if (CHUNK_SIZE.x <= 0 || CHUNK_SIZE.y <= 0 || CHUNK_SIZE.z <= 0) {
		log::system_error("ChunkManager", "CHUNK_SIZE < 0");
		std::exit(1);
	}
	if ((CHUNK_SIZE.x & (CHUNK_SIZE.x - 1)) != 0 || (CHUNK_SIZE.y & (CHUNK_SIZE.y - 1)) != 0 || (CHUNK_SIZE.z & (CHUNK_SIZE.z - 1)) != 0) {
		log::system_error("ChunkManager", "CHUNK_SIZE must be a power of 2");
		std::exit(1);
	}
	perlin_node = FastNoise::New<FastNoise::Perlin>();
	fractal_node = FastNoise::New<FastNoise::FractalFBm>();
	fractal_node->SetSource(perlin_node);
	fractal_node->SetOctaveCount(1);
	fractal_node->SetLacunarity(4.48f);
	fractal_node->SetGain(0.440f);
	fractal_node->SetWeightedStrength(-0.32f);
	compute.setIVec3("CHUNK_SIZE", CHUNK_SIZE);
	glCreateVertexArrays(1, &VAO);
}
ChunkManager::~ChunkManager()
{
	chunks.clear();
	if (VAO)
		glDeleteVertexArrays(1, &VAO);
}

void ChunkManager::update_mesh(Chunk *chunk) noexcept 
{
	Timer update("update_mesh");

#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	/*
	uint zero = 0;
	chunk->counter_ssbo.update_data(&zero, sizeof(uint));
	*/
	if (!chunk->dirty)
        return;

	if (chunk->block_ssbo.id())
		chunk->block_ssbo.update_data(chunk->packed_blocks, sizeof(chunk->packed_blocks));

	chunk->block_ssbo.bind_to_slot(1);
	chunk->face_ssbo.bind_to_slot(2);
	chunk->counter_ssbo.bind_to_slot(3);
	chunk->indirect_ssbo.bind_to_slot(4);

	reset.use();
	uint num_threads = (chunk->counter_ssbo.size() / sizeof(uint) + 63) / 64;
	glDispatchCompute(num_threads, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	compute.use();
	glDispatchCompute(CHUNK_SIZE.x/8, CHUNK_SIZE.y/8, CHUNK_SIZE.z/4);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	chunk->dirty = false;
	indirect.use();
	glDispatchCompute(1, 1, 1);
	glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
}
bool ChunkManager::updateBlock(glm::vec3 world_pos, Block::blocks newType)
{
	Chunk* chunk = getChunk(world_pos);
	if (!chunk)
		return false;
	glm::ivec3 localPos = Chunk::world_to_local(world_pos);

	chunk->setBlockAt(localPos.x, localPos.y, localPos.z, newType);
	update_mesh(chunk);
	return true;
}
Chunk* ChunkManager::getChunk(glm::vec3 world_pos) const
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	auto it = chunks.find(Chunk::world_to_chunk(world_pos));
	if (it != chunks.end())
		return it->second.get();
	else {
#if defined(DEBUG)
		log::system_error("ChunkManager", "Chunk at {} not found!", glm::to_string(worldPos));
#endif
		return nullptr;
	}
}
void ChunkManager::load_around_pos(glm::ivec3 playerChunkPos, unsigned int renderDistance)
{
#if defined(TRACY_ENABLE)
    ZoneScoped;
#endif

    if (renderDistance == 0)
        return;
	Timer chunksTimer("chunk_generation");

    const int side = 2 * renderDistance + 1;
	const int dist = static_cast<int>(renderDistance);

	for (int dz = -dist; dz <= dist; ++dz)
	{
		for (int dy = -dist; dy <= dist; ++dy) 
		{
			for (int dx = -dist; dx <= dist; ++dx)
			{
				glm::ivec3 chunkPos{playerChunkPos.x + dx, playerChunkPos.y + dy, playerChunkPos.z + dz};

				if (chunks.find(chunkPos) != chunks.end())
					continue;

				auto& chunk = chunks[chunkPos] = std::make_unique<Chunk>(chunkPos);

				int offsetX = (dx + renderDistance) * CHUNK_SIZE.x;
				int offsetY = (dy + renderDistance) * CHUNK_SIZE.y;
				int offsetZ = (dz + renderDistance) * CHUNK_SIZE.z;

				chunk->generate(fractal_node, SEED);
				chunk->state = ChunkState::Generated;

				// Optional: queue mesh update instead of immediate
				//dirtyChunks.push_back(chunk.get());
				update_mesh(chunk.get());
			}
		}
	}
}
void ChunkManager::unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance)
{
    const int unloadDistSquared = unloadDistance * unloadDistance;

    // Only iterate over chunks near the edges
    for (auto it = chunks.begin(); it != chunks.end(); )
    {
        glm::vec3 chunkPos = it->first;
		glm::vec3 distance = chunkPos - glm::vec3(playerChunkPos);

        if (glm::length2(distance) > unloadDistSquared)
            it = chunks.erase(it); // Unload chunk
        else
            ++it;
    }
}

void ChunkManager::generate_chunks(glm::vec3 playerPos, unsigned int renderDistance)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	glm::ivec3 playerChunk{Chunk::world_to_chunk(playerPos)};

	if (playerChunk != last_player_chunk_pos) {
		load_around_pos(playerChunk, renderDistance);
		unload_around_pos(playerChunk, renderDistance + 4);
		last_player_chunk_pos = playerChunk;
	}
}
