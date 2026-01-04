#pragma once
#include <glm/matrix.hpp>

struct CameraTemporal {
	glm::mat4 prev_view_proj{1.0f};
	bool      first_frame = true;
};
