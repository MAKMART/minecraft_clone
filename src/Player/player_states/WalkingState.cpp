#include "WalkingState.h"  // Include the header to get WalkingState class declaration

// Entering the Walking State
void WalkingState::enterState(Player& player) {
    player.isWalking = true;
}
// Exiting the Walking State
void WalkingState::exitState(Player& player) {
    player.isWalking = false;
}

// Handling input when in the Walking State
void WalkingState::handleInput(Player& player, float deltaTime) {
    if (!player._camera) return;

    glm::vec3 movement(0.0f);
    float speed = player.walking_speed;

    // Handle movement keys (W, A, S, D, Space for Up)
    if (player.input->isHeld(FORWARD_KEY))
        movement += player._camera->ProcessKeyboard(Camera_Movement::FORWARD, deltaTime, player.walking_speed);
    if (player.input->isHeld(BACKWARD_KEY))
        movement += player._camera->ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime, player.walking_speed);
    if (player.input->isHeld(LEFT_KEY))
        movement += player._camera->ProcessKeyboard(Camera_Movement::LEFT, deltaTime, player.walking_speed);
    if (player.input->isHeld(RIGHT_KEY))
        movement += player._camera->ProcessKeyboard(Camera_Movement::RIGHT, deltaTime, player.walking_speed);
    if (player.input->isHeld(JUMP_KEY))
        player.jump();

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
