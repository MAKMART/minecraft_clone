#pragma once
#include <glm/glm.hpp>
#include "../entity.hpp"

struct CameraController {

	CameraController(Entity target) : target(target) {}

	Entity    target{UINT32_MAX};
	glm::vec3 offset{0.0f, 0.0f, 0.0f};
	bool      third_person   = false;
	float     orbit_distance = 5.0f;
	float     yaw            = -90.0f;
	float     pitch          = 0.0f;
	float     sensitivity    = 0.1f;
};
