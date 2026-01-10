#pragma once
#include <glm/common.hpp>

struct MovementIntent {
    glm::vec3 wish_dir = glm::vec3(0.0f);
    bool jump          = false;
    bool sprint        = false;
    bool crouch        = false;
};
