#pragma once
#include <glm/common.hpp>

struct FrustumVolume {
	struct FrustumPlane {
		glm::vec4 equation; // ax + by + cz + d = 0

		glm::vec3 normal() const
		{
			return glm::vec3(equation);
		}
		float offset() const
		{
			return equation.w;
		}
	};
	std::array<FrustumPlane, 6> planes;
};
