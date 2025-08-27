#pragma once
#include <glm/glm.hpp>
#include "core/aabb.hpp"

struct Collider {
	glm::vec3 halfExtents;
	AABB aabb;
	bool solid = true;                 // optional: non-solid entities
	bool is_on_ground;

	AABB getBoundingBoxAt(const glm::vec3& pos) const {
		return AABB::fromCenterExtent(pos, halfExtents);
	}

	Collider() = default;
	Collider(const glm::vec3 &min, const glm::vec3 &max) : aabb(min, max) {}
	Collider(const glm::vec3& _halfExtents) : halfExtents(_halfExtents) {};
};
