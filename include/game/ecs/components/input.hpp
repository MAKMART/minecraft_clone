#pragma once
#include <glm/glm.hpp>

struct InputComponent {
	// All possible movement directions that can be controlled by an input device
	bool forward  = false;
	bool backward = false;
	bool left     = false;
	bool right    = false;
	bool jump     = false;
	bool sprint   = false;
	bool crouch   = false;

	glm::vec2 mouseDelta{0.0f};
	bool is_primary_pressed = false;
	bool is_ternary_pressed = false;
	bool is_secondary_pressed = false;
};
