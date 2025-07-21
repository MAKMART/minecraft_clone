#include "SwimmingState.h"

void SwimmingState::enterState(Player &player) {
    player.isSwimming = true;
}

void SwimmingState::exitState(Player &player) {
    player.isSwimming = false;
}

void SwimmingState::handleInput(Player &player, float deltaTime) {

    (void)deltaTime;

    glm::vec3 movement(0.0f);

    float speed = player.swimming_speed;

    if (player.input->isHeld(FORWARD_KEY))
        movement += player.getCameraFront() * speed;
    if (player.input->isHeld(BACKWARD_KEY))
        movement -= player.getCameraFront() * speed;
    if (player.input->isHeld(LEFT_KEY))
        movement -= player.getCameraRight() * speed;
    if (player.input->isHeld(RIGHT_KEY))
        movement += player.getCameraRight() * speed;
    //	TODO: Implement these keys specifically for the SwimmingState
    if (player.input->isHeld(GLFW_KEY_SPACE))
        movement += player.getCameraUp() * speed;
    if (player.input->isHeld(GLFW_KEY_LEFT_CONTROL))
        movement -= player.getCameraUp() * speed;
    // Normalize movement and apply speed
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement) * speed;
        player.velocity.x = movement.x;
        player.velocity.z = movement.z;
    } else {
        player.velocity.x = 0.0f;
        player.velocity.z = 0.0f;
    }
}
