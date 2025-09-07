#pragma once
#include <glm/glm.hpp>

struct Position {
	glm::vec3 value{0.0f};
	Position() = default;
	Position(float x, float y, float z) : value(x, y, z)
	{
	}
	explicit Position(const glm::vec3& pos) : value(pos)
	{
	}
};
