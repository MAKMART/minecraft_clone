#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "game/ecs/entity.hpp"

struct Transform {
	Transform() = default;
	explicit Transform(const glm::vec3& pos) : pos(pos) {}
	Transform(const glm::vec3& pos, const glm::quat& quat) : pos(pos), rot(quat) {}
	Transform(const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale) : pos(pos), rot(quat), scale(scale) {}

	glm::vec3 pos{0.0f};
	glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity
	glm::vec3 scale{1.0f};
	Entity    parent = {UINT32_MAX}; // invalid if no parent
};
