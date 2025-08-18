#include "player/player_states/RunningState.h"

void RunningState::enterState(Player &player) {
    player.isRunning = true;
}

void RunningState::exitState(Player &player) {
    player.isRunning = false;
}

void RunningState::handleInput(Player &player, float deltaTime) {

    (void)deltaTime;

    glm::vec3 movement(0.0f);

    const auto &input = player.input;
    player.running_speed = player.walking_speed + player.running_speed_increment;
    float speed = player.running_speed;

    glm::vec3 front = player.getCameraFront();
    glm::vec3 right = player.getCameraRight();

    // Flatten movement to horizontal plane
    front.y = 0.0f;
    right.y = 0.0f;
    front = glm::normalize(front);
    right = glm::normalize(right);

    if (input->isHeld(FORWARD_KEY))
        movement += front;
    if (input->isHeld(BACKWARD_KEY))
        movement -= front;
    if (input->isHeld(LEFT_KEY))
        movement -= right;
    if (input->isHeld(RIGHT_KEY))
        movement += right;

    // Normalize direction vector and apply speed
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement) * speed;
        player.velocity.x = movement.x;
        player.velocity.z = movement.z;
    } else {
        player.velocity.x = 0.0f;
        player.velocity.z = 0.0f;
    }

    if (input->isHeld(JUMP_KEY))
        player.jump();
}
