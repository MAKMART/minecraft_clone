#include "player/player_states/FlyingState.h"
void FlyingState::enterState(Player &player) {
    player.velocity = glm::vec3(0.0f);
    player.isFlying = true;
    player.isOnGround = false;
}

void FlyingState::exitState(Player &player) {
    player.isFlying = false;
}

void FlyingState::handleInput(Player &player, float deltaTime) {

    (void)deltaTime;

    glm::vec3 movement(0.0f);
    float speed = player.flying_speed;

    if (player.input->isHeld(FORWARD_KEY))
        movement += player.getCameraFront() * speed;
    if (player.input->isHeld(BACKWARD_KEY))
        movement -= player.getCameraFront() * speed;
    if (player.input->isHeld(LEFT_KEY))
        movement -= player.getCameraRight() * speed;
    if (player.input->isHeld(RIGHT_KEY))
        movement += player.getCameraRight() * speed;
    if (player.input->isHeld(DOWN_KEY))
        movement -= player.getCameraController().getWorldUp() * speed;
    if (player.input->isHeld(UP_KEY))
        movement += player.getCameraController().getWorldUp() * speed;

    // Normalize movement and apply speed
    if (glm::length(movement) > 0.0f) {
        movement = glm::normalize(movement) * speed;
        player.velocity = movement;
    } else {
        player.velocity = glm::vec3(0.0f);
    }
}
