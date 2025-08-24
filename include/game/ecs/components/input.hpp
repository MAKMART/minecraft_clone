#pragma once
#include <glm/glm.hpp>

struct InputComponent {
    // High-level controls (not raw keycodes)
    bool forward  = false;
    bool backward = false;
    bool left     = false;
    bool right    = false;
    bool jump     = false;
    bool sprint   = false;
    bool crouch   = false;

    glm::vec2 mouseDelta{0.0f};
};
