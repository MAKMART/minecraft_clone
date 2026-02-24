#include "chunk/chunk.hpp"
#include "core/defines.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

Chunk::Chunk(const glm::ivec3& pos)
	: position(pos), 
	changed(true),           // Force initial upload
	in_dirty_list(false), 
	faces_offset(0), 
	current_mesh_bytes(0),
	visible_face_count(0)
{
	glm::vec3 world = chunk_to_world(position);
	aabb = AABB(world, world + glm::vec3(CHUNK_SIZE));

	block_ssbo = SSBO::Dynamic(nullptr, sizeof(block_types));
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

	glm::vec3 chunk_world_origin = Chunk::chunk_to_world(position);

	// ─────────────────────────────────────────────────────────────
    //               TUNABLE TERRAIN PARAMETERS
    // ─────────────────────────────────────────────────────────────

    // Height range & vertical distribution
    constexpr float    BASE_HEIGHT           = 64.0f;      // Approximate "sea level" / reference height
    constexpr int      MAX_TERRAIN_HEIGHT    = 500;        // Max possible mountain peak height (world space)
    constexpr float    MIN_TERRAIN_HEIGHT    = -200.0f;    // Lowest possible terrain floor (world space)

    // Noise → height mapping
    constexpr float    NOISE_TO_HEIGHT_SCALE = static_cast<float>(MAX_TERRAIN_HEIGHT - MIN_TERRAIN_HEIGHT);
    constexpr float    NOISE_VERTICAL_OFFSET = MIN_TERRAIN_HEIGHT;  // Shifts the [-1..1] → desired range

    // Surface layering
    constexpr int      DIRT_DEPTH            = 3;          // How many layers of dirt under grass
    constexpr int      BEACH_DEPTH           = 1;          // (currently unused — can be used for sand/gravel later)

    // Noise characteristics
    constexpr float    NOISE_FREQUENCY       = 1.0f / 150.0f;  // Smaller = larger features (150 = ~150 block wide hills)
    constexpr float    NOISE_AMPLITUDE       = 1.0f;           // Usually keep at 1.0f for full [-1..1] range

    // Optional: vertical stretching (make mountains taller / valleys deeper)
    // constexpr float NOISE_VERTICAL_SQUASH = 1.0f;  // >1 = taller features, <1 = flatter

    // ─────────────────────────────────────────────────────────────

	int noise_index = 0;

	for (int lx = 0; lx < CHUNK_SIZE.x; ++lx)
	{
		for (int lz = 0; lz < CHUNK_SIZE.z; ++lz)
		{
			const float wx = chunk_world_origin.x + static_cast<float>(lx);
			const float wz = chunk_world_origin.z + static_cast<float>(lz);

			// Sample noise — normalized to [0,1]
			float raw_noise = noise_node->GenSingle2D(
					wx * NOISE_FREQUENCY,
					wz * NOISE_FREQUENCY,
					SEED
					);

			// Map FastNoise's typical [-1..1] range → [0..1]
			float normalized = (raw_noise + 1.0f) * 0.5f;

			// Convert to world height
			float world_height_f = BASE_HEIGHT + (normalized * NOISE_TO_HEIGHT_SCALE + NOISE_VERTICAL_OFFSET);
			int world_height = static_cast<int>(std::round(world_height_f));

			// Convert to local chunk Y
			int local_surface_y = world_height - static_cast<int>(chunk_world_origin.y);

			for (int ly = 0; ly < CHUNK_SIZE.y; ++ly)
			{
				int idx = get_index(lx, ly, lz);
				Block::blocks type = Block::blocks::AIR;

				if (ly <= local_surface_y)
				{
					if (ly == local_surface_y)
					{
						type = Block::blocks::GRASS;
					}
					else if (ly >= local_surface_y - DIRT_DEPTH)
					{
						type = Block::blocks::DIRT;
					}
					else
					{
						type = Block::blocks::STONE;
					}

					++non_air_count;
				}

				block_types[idx] = static_cast<std::uint8_t>(type);
			}
		}
	}

	this->changed = true;

}
