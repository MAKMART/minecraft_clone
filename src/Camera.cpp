#include "Camera.h"
#include <glm/common.hpp>
#include <optional>

// Constructor
Camera::Camera(glm::vec3 position, glm::vec3 up, glm::quat orient)
    : orientation(orient), trackMouse(true) {
    Position = position;
    WorldUp = up;
    Target = position; // Initially, target is the same as position
}

// Constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, glm::quat orient)
    : orientation(orient), trackMouse(true) {
    Position = glm::vec3(posX, posY, posZ);
    WorldUp = glm::vec3(upX, upY, upZ);
    Target = Position;
}

void Camera::setPosition(const glm::vec3 &newPos) {
    if (std::isfinite(newPos.x) && std::isfinite(newPos.y) && std::isfinite(newPos.z)) {
        Position = newPos;
        if (isThirdPerson)
            UpdateThirdPerson(Target); // Maintain third-person consistency
    }
}

// Returns the view matrix
glm::mat4 Camera::GetViewMatrix() const {
    if (isThirdPerson) {
        return glm::lookAt(Position, Target, WorldUp);
    } else {
        return glm::lookAt(Position, Position + getFront(), WorldUp);
    }
}

// Returns the projection matrix
glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::perspective(glm::radians(FOV), aspectRatio, NEAR_PLANE, FAR_PLANE);
}

// Process keyboard input and return updated velocity
glm::vec3 Camera::ApplyKeyboardPhysics(Camera_Movement direction, float deltaTime, glm::vec3 velocity) {
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
        return;
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    // Yaw and pitch rotations
    glm::quat yawRot = glm::angleAxis(glm::radians(-xoffset), glm::vec3(0, 1, 0));  // Global Y-axis
    glm::quat pitchRot = glm::angleAxis(glm::radians(yoffset), glm::vec3(1, 0, 0)); // Local right axis
    orientation = yawRot * orientation * pitchRot;
    orientation = glm::normalize(orientation);

    // Correct roll by aligning up vector
    glm::vec3 up = getUp();
    glm::vec3 right = getRight();
    glm::vec3 front = getFront();
    // Recompute up to be orthogonal to front and aligned with WorldUp
    right = glm::normalize(glm::cross(WorldUp, front));
    up = glm::normalize(glm::cross(front, right));
    // Create quaternion to align current orientation
    orientation = glm::quatLookAt(front, up);
    orientation = glm::normalize(orientation);

    // Pitch clamping
    if (constrainPitch) {
        glm::vec3 front = getFront();
        float pitch = glm::degrees(std::asin(front.y));
        if (pitch > 89.0f) {
            float delta = glm::radians(pitch - 89.0f);
            glm::quat correction = glm::angleAxis(delta, getRight());
            orientation = orientation * correction;
        } else if (pitch < -89.0f) {
            float delta = glm::radians(pitch + 89.0f);
            glm::quat correction = glm::angleAxis(delta, getRight());
            orientation = orientation * correction;
        }
        orientation = glm::normalize(orientation);
    }

    if (isThirdPerson) {
        UpdateThirdPerson(Target);
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
        constexpr float MIN_SPEED = 0.1f;
        MovementSpeed = glm::max(MIN_SPEED, MovementSpeed + yoffset * scroll_speed_multiplier);
    }
}
// Update third-person camera position
void Camera::UpdateThirdPerson(const glm::vec3 &target) {
    Target = target;
    // Offset vector (camera behind target along -Z in local space)
    glm::vec3 offset = glm::vec3(0, 0, Distance); // +Z to place camera behind target
    // Rotate offset by orientation
    offset = orientation * offset;
    // Set position
    Position = Target + offset;
}
// Switch to third-person mode
void Camera::SwitchToThirdPerson(const glm::vec3 &target, std::optional<float> distance) {
    isThirdPerson = true;
    Target = target;
    if (distance.has_value())
        Distance = distance.value();
    UpdateThirdPerson(Target);
}

// Switch to first-person mode
void Camera::SwitchToFirstPerson(const glm::vec3 &position) {
    isThirdPerson = false;
    Position = position;
    Target = position;
}
std::array<Camera::FrustumPlane, 6> Camera::extractFrustumPlanes() const {
    glm::mat4 viewProj = GetProjectionMatrix() * GetViewMatrix();
    std::array<FrustumPlane, 6> planes;

    // Left plane
    planes[0].normal.x = viewProj[0][3] + viewProj[0][0];
    planes[0].normal.y = viewProj[1][3] + viewProj[1][0];
    planes[0].normal.z = viewProj[2][3] + viewProj[2][0];
    planes[0].distance = viewProj[3][3] + viewProj[3][0];

    // Right plane
    planes[1].normal.x = viewProj[0][3] - viewProj[0][0];
    planes[1].normal.y = viewProj[1][3] - viewProj[1][0];
    planes[1].normal.z = viewProj[2][3] - viewProj[2][0];
    planes[1].distance = viewProj[3][3] - viewProj[3][0];

    // Bottom plane
    planes[2].normal.x = viewProj[0][3] + viewProj[0][1];
    planes[2].normal.y = viewProj[1][3] + viewProj[1][1];
    planes[2].normal.z = viewProj[2][3] + viewProj[2][1];
    planes[2].distance = viewProj[3][3] + viewProj[3][1];

    // Top plane
    planes[3].normal.x = viewProj[0][3] - viewProj[0][1];
    planes[3].normal.y = viewProj[1][3] - viewProj[1][1];
    planes[3].normal.z = viewProj[2][3] - viewProj[2][1];
    planes[3].distance = viewProj[3][3] - viewProj[3][1];

    // Near plane
    planes[4].normal.x = viewProj[0][3] + viewProj[0][2];
    planes[4].normal.y = viewProj[1][3] + viewProj[1][2];
    planes[4].normal.z = viewProj[2][3] + viewProj[2][2];
    planes[4].distance = viewProj[3][3] + viewProj[3][2];

    // Far plane
    planes[5].normal.x = viewProj[0][3] - viewProj[0][2];
    planes[5].normal.y = viewProj[1][3] - viewProj[1][2];
    planes[5].normal.z = viewProj[2][3] - viewProj[2][2];
    planes[5].distance = viewProj[3][3] - viewProj[3][2];

    // Normalize planes
    for (auto &plane : planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.distance /= length;
    }

    return planes;
}
