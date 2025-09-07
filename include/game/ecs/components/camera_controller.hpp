#pragma once
#include <glm/glm.hpp>
#include "game/ecs/entity.hpp"

struct CameraController {

	explicit CameraController(Entity _target) : target(_target) {}
	CameraController(Entity _target, const glm::vec3& _offset) : target(_target), offset(_offset) {}

	Entity    target{UINT32_MAX};
	glm::vec3 offset{0.0f, 0.0f, 0.0f};
	bool      third_person   = false;
	float     orbit_distance = 5.0f;
	float     yaw            = -90.0f;
	float     pitch          = 0.0f;
	float     sensitivity    = 0.25f;
};
