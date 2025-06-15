#include "Camera.h"

// Constructor
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : trackMouse(true) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    Target = position; // Initially, target is the same as position
    Distance = 5.0f;   // Default third-person distance
}

// Constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : trackMouse(true) {
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Yaw = yaw;
    Pitch = pitch;
    Target = Position;
    Distance = 5.0f;
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 Camera::GetViewMatrix() const {
    if (isThirdPerson) {
        return glm::lookAt(Position, Target, getUp());
    } else {
        return glm::lookAt(Position, Position + getFront(), getUp());
    }
}

// return the projection matrix
glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(FOV), aspectRatio, NEAR_PLANE, FAR_PLANE);
}

// Process keyboard input and return updated velocity
glm::vec3 Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime, glm::vec3 velocity) {
    float speed = MovementSpeed * deltaTime;
    if (direction == Camera_Movement::FORWARD)
        velocity += getFront() * speed;
    if (direction == Camera_Movement::BACKWARD)
        velocity -= getFront() * speed;
    if (direction == Camera_Movement::LEFT)
        velocity -= getRight() * speed;
    if (direction == Camera_Movement::RIGHT)
        velocity += getRight() * speed;
    if (direction == Camera_Movement::UP)
        velocity += getUp() * speed;
    if (direction == Camera_Movement::DOWN)
        velocity -= getUp() * speed;

    // Update camera position based on velocity (only in first-person or if not third-person controlling player)
    if (!isThirdPerson || !trackMouse) {
        Position += velocity * deltaTime;
    }

    return velocity;
}
// --- THIS IS FOR FACING DEPENDENT MOVEMENT ---
/*glm::vec3 Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime, float movement_velocity)
{
    glm::vec3 movement(0.0f); // Initialize movement vector
    float speed = movement_velocity * deltaTime;

    if (direction == FORWARD)
        movement += glm::vec3(0.0f, 0.0f, -1.0f) * speed; // Ignore Y component
    if (direction == BACKWARD)
        movement -= glm::vec3(0.0f, 0.0f, 1.0f) * speed; // Ignore Y component
    if (direction == LEFT)
        movement -= glm::vec3(getRight().x, 0.0f, getRight().z) * speed; // Ignore Y component
    if (direction == RIGHT)
        movement += glm::vec3(getRight().x, 0.0f, getRight().z) * speed; // Ignore Y component

    // Normalize movement to maintain constant speed in diagonal directions
    if (glm::length(movement) > 0.0f)
        movement = glm::normalize(movement) * speed;

    return movement; // Return the calculated movement instead of modifying Position
}*/
glm::vec3 Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime, float movement_velocity) {
    glm::vec3 movement(0.0f);
    float speed = movement_velocity * deltaTime;

    // Flatten the forward vector to prevent slow movement when looking down/up
    glm::vec3 flatForward = glm::normalize(glm::vec3(getFront().x, 0.0f, getFront().z));
    glm::vec3 right = glm::normalize(glm::vec3(getRight().x, 0.0f, getRight().z));

    if (direction == Camera_Movement::FORWARD)
        movement += flatForward * speed;
    if (direction == Camera_Movement::BACKWARD)
        movement -= flatForward * speed;
    if (direction == Camera_Movement::LEFT)
        movement -= right * speed;
    if (direction == Camera_Movement::RIGHT)
        movement += right * speed;

    // Keep movement speed consistent in diagonal movement
    if (glm::length(movement) > 0.0f)
        movement = glm::normalize(movement) * speed;

    return movement;
}

// Process mouse movement input
void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    if (!trackMouse)
        return; // Mouse tracking is disabled; ignore mouse movement

    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    if (isThirdPerson) {
        // Rotate around the target (player) in third-person
        // Non-inverted behavior: up moves camera up, down moves camera down
        Yaw -= xoffset;   // Horizontal rotation (left/right, non-inverted)
        Pitch -= yoffset; // Vertical rotation (up/down, non-inverted)

        // Constrain pitch to avoid flipping
        if (constrainPitch) {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // Update camera position based on new rotation
        UpdateThirdPerson(Target);
    } else {
        // Standard first-person rotation
        // Non-inverted behavior: up moves camera up, down moves camera down
        Yaw += xoffset;   // Horizontal rotation (left/right, non-inverted)
        Pitch += yoffset; // Vertical rotation (up/down, non-inverted)

        if (constrainPitch) {
            if (Pitch >= 89.0f)
                Pitch = 89.0f;
            if (Pitch <= -89.0f)
                Pitch = -89.0f;
        }
    }
}

// Process mouse scroll input
void Camera::ProcessMouseScroll(float yoffset) {
    if (isThirdPerson) {
        // Adjust camera distance in third-person
        float scroll_speed_multiplier = 1.0f;
        Distance -= yoffset * scroll_speed_multiplier;
        Distance = glm::max(1.0f, Distance); // Prevent getting too close
        UpdateThirdPerson(Target);
    } else {
        // Adjust movement speed in first-person
        float scroll_speed_multiplier = 1.0f;
        MovementSpeed += yoffset * scroll_speed_multiplier;
        if (MovementSpeed < 0)
            MovementSpeed = 0;
    }
}
// Update third-person camera position
void Camera::UpdateThirdPerson(const glm::vec3 &target) {
    Target = target;

    // Convert angles to radians
    float radYaw = glm::radians(Yaw);
    float radPitch = glm::radians(Pitch);

    // Calculate camera position in spherical coordinates around the target
    float x = Target.x + Distance * cos(radPitch) * sin(radYaw);
    float y = Target.y + Distance * sin(radPitch);
    float z = Target.z + Distance * cos(radPitch) * cos(radYaw);

    Position = glm::vec3(x, y, z);
}

// Switch to third-person mode
void Camera::SwitchToThirdPerson(const glm::vec3 &target, float distance, float yaw, float pitch) {
    isThirdPerson = true;
    Target = target;
    Distance = distance;

    // Use the current Yaw and Pitch from first-person mode if not provided
    if (yaw == -90.0f && pitch == 20.0f) { // Default values indicate no override
        // Keep the current orientation (Yaw and Pitch) from first-person
        Yaw = Yaw; // Already set, but explicit for clarity
        Pitch = Pitch;
    } else {
        // Use provided yaw and pitch if specified
        Yaw = yaw;
        Pitch = pitch;
    }

    UpdateThirdPerson(Target);
}

// Switch to first-person mode
void Camera::SwitchToFirstPerson(const glm::vec3 &position) {
    isThirdPerson = false;
    Position = position;
    Target = position;                         // Reset target to match position
    getFront() = glm::vec3(0.0f, 0.0f, -1.0f); // Reset front for first-person
}
