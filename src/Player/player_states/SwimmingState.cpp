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
        float speed = player.swimming_speed * deltaTime;

        if (player.input->isHeld(FORWARD_KEY))
            movement += player._camera->Front * speed;
        if (player.input->isHeld(BACKWARD_KEY))
            movement -= player._camera->Front * speed;
        if (player.input->isHeld(LEFT_KEY))
            movement -= player._camera->Right * speed;
        if (player.input->isHeld(RIGHT_KEY))
            movement += player._camera->Right * speed;
	//	TODO: Implement these keys specifically for the SwimmingState
        if (player.input->isHeld(GLFW_KEY_SPACE))
            movement += player._camera->Up * speed;
        if (player.input->isHeld(GLFW_KEY_LEFT_CONTROL))
            movement -= player._camera->Up * speed;

        player.pendingMovement += movement;
}
