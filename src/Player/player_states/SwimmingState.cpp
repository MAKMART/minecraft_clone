#include "SwimmingState.h"

// Entering the Walking State
void SwimmingState::enterState(Player& player) {
    player.isSwimming = true;
}

// Exiting the Walking State
void SwimmingState::exitState(Player& player) {
    player.isSwimming = false;    
}

// Handling input when in the Walking State
void SwimmingState::handleInput(Player& player, float deltaTime) {
    glm::vec3 movement(0.0f);
    float speed = player.swimming_speed;

    if (player.input->isHeld(FORWARD_KEY))
	movement += player._camera->getFront() * speed;
    if (player.input->isHeld(BACKWARD_KEY))
	movement -= player._camera->getFront() * speed;
    if (player.input->isHeld(LEFT_KEY))
	movement -= player._camera->getRight() * speed;
    if (player.input->isHeld(RIGHT_KEY))
	movement += player._camera->getRight() * speed;
    //	TODO: Implement these keys specifically for the SwimmingState
    if (player.input->isHeld(GLFW_KEY_SPACE))
	movement += player._camera->getUp() * speed;
    if (player.input->isHeld(GLFW_KEY_LEFT_CONTROL))
	movement -= player._camera->getUp() * speed;
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
