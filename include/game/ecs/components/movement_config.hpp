#pragma once

struct MovementConfig {
    bool can_jump = true;
    bool can_walk = true;
    bool can_run = true;
    bool can_crouch = true;
    bool can_fly = false;
    float jump_height = 1.8f;
    float walk_speed = 5.0f;
    float run_speed = 7.5f;
    float crouch_speed = 2.5f;
    float fly_speed = 10.0f;
};
