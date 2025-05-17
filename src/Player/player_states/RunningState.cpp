#include "RunningState.h"

void RunningState::enterState(Player& player) {
    player.isRunning = true;
}

void RunningState::exitState(Player& player) {
    player.isRunning = false;
}

void RunningState::handleInput(Player& player, float deltaTime) {
    if (!player._camera) return;

    glm::vec3 movement(0.0f);
    player.running_speed = player.walking_speed + player.running_speed_increment;
    float speed = player.running_speed;

    if (player.input->isHeld(FORWARD_KEY))
	movement += player._camera->ProcessKeyboard(Camera_Movement::FORWARD, deltaTime, player.running_speed);
    if (player.input->isHeld(BACKWARD_KEY))
	movement += player._camera->ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime, player.running_speed);
    if (player.input->isHeld(LEFT_KEY))
	movement += player._camera->ProcessKeyboard(Camera_Movement::LEFT, deltaTime, player.running_speed);
    if (player.input->isHeld(RIGHT_KEY))
	movement += player._camera->ProcessKeyboard(Camera_Movement::RIGHT, deltaTime, player.running_speed);
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
