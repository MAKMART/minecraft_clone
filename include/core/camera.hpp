#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class _Camera
{
      public:
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

	glm::mat4                   computeViewMatrix(const glm::vec3& camPos, const glm::quat& orientation) const;
	glm::mat4                   computeProjectionMatrix(float fov, float aspect, float nearPlane, float farPlane) const;
	std::array<FrustumPlane, 6> extractFrustumPlanes(const glm::mat4& view, const glm::mat4& projection) const;
};
