#pragma once
#include <glm/glm.hpp>

struct Velocity {
	glm::vec3 value{0.0f};
	Velocity() = default;
	Velocity(float x, float y, float z) : value(x, y, z)
	{
	}
	explicit Velocity(const glm::vec3& vel) : value(vel)
	{
	}
};
