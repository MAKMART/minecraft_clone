#include "chunk/chunk_manager.hpp"
#include <cmath>
#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#include "core/timer.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

ChunkManager::ChunkManager(std::optional<int> seed)
    : shader("Chunk", CHUNK_VERTEX_SHADER_DIRECTORY, CHUNK_FRAGMENT_SHADER_DIRECTORY),
      waterShader("Water", WATER_VERTEX_SHADER_DIRECTORY, WATER_FRAGMENT_SHADER_DIRECTORY),
	  compute("Chunk_compute", SHADERS_DIRECTORY / "chunk.comp")
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
	fractal_node->SetOctaveCount(2);
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

bool ChunkManager::updateBlock(glm::vec3 world_pos, Block::blocks newType)
{
	Chunk* chunk = getChunk(world_pos);
	if (!chunk)
		return false;
	glm::ivec3 localPos = Chunk::world_to_local(world_pos);

	chunk->setBlockAt(localPos.x, localPos.y, localPos.z, newType);
	compute.use();
	chunk->updateMesh();
	return true;
}
Chunk* ChunkManager::getChunk(glm::vec3 world_pos) const
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	auto it = chunks.find(Chunk::world_to_chunk(world_pos));
	if (it != chunks.end())
		return it->second.get(); // Return raw pointer (Chunk*)
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

    // Compute the bounds of chunks to load
    glm::ivec2 regionSize = {side * CHUNK_SIZE.x, side * CHUNK_SIZE.z};

    // Determine the origin of the noise grid
    glm::ivec3 noiseOrigin = {
        (playerChunkPos.x - renderDistance) * CHUNK_SIZE.x,
        0,
        (playerChunkPos.z - renderDistance) * CHUNK_SIZE.z
    };
	{
		Timer noiseTimer("Noise precomputation");
		// Precompute noise only if necessary
		if (lastRegionSize != regionSize || lastNoiseOrigin != noiseOrigin)
		{
			cachedNoiseRegion.resize(regionSize.x * regionSize.y);

			constexpr float step = 0.4f;

			fractal_node->GenUniformGrid2D(
					cachedNoiseRegion.data(),
					static_cast<float>(noiseOrigin.x),
					static_cast<float>(noiseOrigin.z),
					regionSize.x,
					regionSize.y,
					step,
					step,
					SEED
					);

			lastRegionSize = regionSize;
			lastNoiseOrigin = noiseOrigin;
		}
	}

	compute.use();
    for (int dz = -static_cast<int>(renderDistance); dz <= static_cast<int>(renderDistance); ++dz)
    {
        for (int dx = -static_cast<int>(renderDistance); dx <= static_cast<int>(renderDistance); ++dx)
        {
            glm::ivec3 chunkPos{playerChunkPos.x + dx, 0, playerChunkPos.z + dz};

            if (chunks.find(chunkPos) != chunks.end())
                continue; // Chunk already loaded

            // Insert new chunk
            auto& chunk = chunks[chunkPos] = std::make_unique<Chunk>(chunkPos);

            int offsetX = (dx + renderDistance) * CHUNK_SIZE.x;
            int offsetZ = (dz + renderDistance) * CHUNK_SIZE.z;

            // Generate terrain for this chunk
            chunk->generate(std::span{cachedNoiseRegion}, regionSize.x, offsetX, offsetZ);
            chunk->state = ChunkState::Generated;

            // Optional: queue mesh update instead of immediate
            //dirtyChunks.push_back(chunk.get());
			chunk->updateMesh();
        }
    }
}
void ChunkManager::unload_around_pos(glm::ivec3 playerChunkPos, unsigned int unloadDistance)
{
    const int unloadDistSquared = unloadDistance * unloadDistance;

    // Only iterate over chunks near the edges
    for (auto it = chunks.begin(); it != chunks.end(); )
    {
        glm::ivec3 chunkPos = it->first;
        int dx = chunkPos.x - playerChunkPos.x;
        int dz = chunkPos.z - playerChunkPos.z;

        if ((dx * dx + dz * dz) > unloadDistSquared)
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

	if (playerChunk.x != last_player_chunk_pos.x || playerChunk.z != last_player_chunk_pos.z) {
		load_around_pos(playerChunk, renderDistance);
		unload_around_pos(playerChunk, renderDistance + 4);
		last_player_chunk_pos.x = playerChunk.x;
		last_player_chunk_pos.z = playerChunk.z;
	}
}
