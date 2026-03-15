module;
//#include <optional>
//#include <memory>
#include <FastNoise/FastNoise.h>
export module noise;

import std;
import core;
import glm;

export class NoiseSystem
{
	public:
		constexpr int get_seed() const noexcept { return seed; }
		const float* get_noise() const noexcept { return chunk_noise; }

		NoiseSystem(std::optional<int> _seed) noexcept : seed(_seed.value_or(seed))
		{
			base = FastNoise::New<FastNoise::Simplex>();
			terrain = FastNoise::New<FastNoise::FractalFBm>();
			terrain->SetSource(base);
			terrain->SetOctaveCount(5);
			terrain->SetGain(0.5f);
			terrain->SetLacunarity(2.0f);
		}
		float sample_height(float x, float z) const noexcept
		{
			float scale = 0.005f;
			//float n = terrain->GenSingle2D(x * scale, z * scale);

			//return n * 80.0f; // terrain amplitude
			return scale * 80.0f;
		}
		float sample_3d(float x, float y, float z) const noexcept
		{
			float scale = 0.02f;
			//return base->GenSingle3D(x * scale, y * scale, z * scale);
			return scale;
		}
		auto gen_grid_2d(const glm::ivec2& world_origin) noexcept
		{
				return terrain->GenUniformGrid2D(
					chunk_noise,
					world_origin.x, world_origin.y, // Start at world coordinates
					CHUNK_SIZE.x, CHUNK_SIZE.z,               // Size of one chunk
					NOISE_FREQUENCY, 
					NOISE_FREQUENCY, 
					seed
					);
		}
	private:
		int seed = 123456;
		FastNoise::SmartNode<FastNoise::FractalFBm> terrain;
		FastNoise::SmartNode<FastNoise::Simplex> base;
		static constexpr float    NOISE_FREQUENCY       = 1.0f / 350.0f;  // Smaller = larger features (150 = ~150 block wide hills)
		alignas(16) float chunk_noise[CHUNK_SIZE.x * CHUNK_SIZE.z];
};
