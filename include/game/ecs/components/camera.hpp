#pragma once
#include <glm/matrix.hpp>

struct Camera {
	float     fov          = 90.0f; // DEG
	float     near_plane   = 0.0001f;
	float     far_plane    = 100000.0f;
	float     aspect_ratio = 16.0f / 9.0f;
	glm::mat4 viewMatrix{1.0f};
	glm::mat4 projectionMatrix{1.0f};

	glm::vec3 forward{0.0f, 0.0f, -1.0f};
	glm::vec3 up{0.0f, 1.0f, 0.0f};
	glm::vec3 right{1.0f, 0.0f, 0.0f};
};
