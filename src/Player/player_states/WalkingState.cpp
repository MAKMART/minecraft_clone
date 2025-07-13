#include "WalkingState.h" // Include the header to get WalkingState class declaration

// Entering the Walking State
void WalkingState::enterState(Player &player) {
    player.isWalking = true;
}
// Exiting the Walking State
void WalkingState::exitState(Player &player) {
    player.isWalking = false;
}

// Handling input when in the Walking State
void WalkingState::handleInput(Player &player, float deltaTime) {

    glm::vec3 movement(0.0f);

    const auto &input = player.input;
    float speed = player.walking_speed;

    // Use camCtrl's direction vectors instead of _camera
    glm::vec3 front = player.getCameraFront();
    glm::vec3 right = player.getCameraRight();

    // Flatten movement to horizontal plane
    front.y = 0.0f;
    right.y = 0.0f;
    front = glm::normalize(front);
    right = glm::normalize(right);

    // WASD Movement
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

    // Jump
    if (input->isHeld(JUMP_KEY))
        player.jump();
}
