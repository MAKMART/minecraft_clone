#include "FlyingState.h"
void FlyingState::enterState(Player& player) {
    player.velocity = glm::vec3(0.0f);
    player.isFlying = true;
    player.isOnGround = false;
}

void FlyingState::exitState(Player& player) {
    player.isFlying = false;
}

void FlyingState::handleInput(Player& player, float deltaTime) {
    glm::vec3 movement(0.0f);
    float speed = player.flying_speed * deltaTime;

    if (player.input->isHeld(FORWARD_KEY))
	movement += player._camera->Front * speed;
    if (player.input->isHeld(BACKWARD_KEY))
	movement -= player._camera->Front * speed;
    if (player.input->isHeld(LEFT_KEY))
	movement -= player._camera->Right * speed;
    if (player.input->isHeld(RIGHT_KEY))
	movement += player._camera->Right * speed;
    if (player.input->isHeld(DOWN_KEY))
	movement -= player._camera->Up * speed;
    if (player.input->isHeld(UP_KEY))
	movement += player._camera->Up * speed;

    // Normalize movement if there is any
    if (glm::length(movement) > 0.0f)
        movement = glm::normalize(movement) * speed;

    // Apply the pending movement (only in the XZ plane)
    player.pendingMovement += movement;
}
