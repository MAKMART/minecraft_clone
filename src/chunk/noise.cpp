module noise;
//#include <FastNoise/FastNoise.h>
//#include <optional>
//#include <memory>
#include "core/defines.hpp"


struct NoiseSystem::impl
{
	/*
	FastNoise::SmartNode<FastNoise::FractalFBm> terrain;
	FastNoise::SmartNode<FastNoise::Simplex> base;

	impl(int seed)
	{
		base = FastNoise::New<FastNoise::Simplex>();

		terrain = FastNoise::New<FastNoise::FractalFBm>();
		terrain->SetSource(base);
		terrain->SetOctaveCount(5);
		terrain->SetGain(0.5f);
		terrain->SetLacunarity(2.0f);


		terrain->SetSeed(seed);
	}
	*/
}

NoiseSystem::NoiseSystem(std::optional<int> _seed)
{
    int s = _seed.value_or(seed);
	seed = s;
    p = std::make_unique<impl>(s);
}
float NoiseSystem::sample_height(float x, float z) const noexcept
{
    float scale = 0.005f;
    float n = p->terrain->GenSingle2D(x * scale, z * scale);

    return n * 80.0f; // terrain amplitude
}
float NoiseSystem::sample_3d(float x, float y, float z) const noexcept
{
    float scale = 0.02f;
    return p->base->GenSingle3D(x * scale, y * scale, z * scale);
}
void NoiseSystem::gen_grid_2d(const glm::ivec2& world_origin) const noexcept
{

	auto min_max = noise.GenUniformGrid2D(
			chunk_noise,
			world_origin.x, world_origin.z, // Start at world coordinates
			CHUNK_SIZE.x, CHUNK_SIZE.z,               // Size of one chunk
			NOISE_FREQUENCY, 
			NOISE_FREQUENCY, 
			seed
			);
}
