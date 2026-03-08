module;
#include <optional>
#include <memory>
#include "core/defines.hpp"

export module noise;

import glm;

export class NoiseSystem
{
	public:
		NoiseSystem(std::optional<int> _seed = std::nullopt) noexcept;

		float sample_height(float x, float z) const noexcept;
		float sample_3d(float x, float y, float z) const noexcept;
		constexpr int get_seed() const noexcept { return seed; }
		const float* get_noise() const noexcept { return chunk_noise; }
		void gen_grid_2d(const glm::ivec2& world_origin) const noexcept;
	private:
		static constexpr int seed = 123456;
		struct impl;
		std::unique_ptr<impl> p;
		static constexpr float    NOISE_FREQUENCY       = 1.0f / 350.0f;  // Smaller = larger features (150 = ~150 block wide hills)
		alignas(16) float chunk_noise[CHUNK_SIZE.x * CHUNK_SIZE.z];
};
