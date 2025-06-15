#pragma once
#include "AABB.h"
#include "Player.h"
#include <GL/glew.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>

enum class Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};

// Default camera values
const glm::quat DEFAULT_ORIENTATION = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion (no rotation)

class Camera {
  public:
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), glm::quat orient = DEFAULT_ORIENTATION);

    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, glm::quat orient = DEFAULT_ORIENTATION);
    ~Camera() = default;

    glm::vec3 &getPosition() {
        return Position;
    }
    glm::quat &getOrientation() {
        return orientation;
    }
    glm::vec3 &getWorldUp() {
        return WorldUp;
    }

    void setPosition(const glm::vec3& newPos);

    void setMouseTracking(bool enabled) { trackMouse = enabled; }
    void setAspectRatio(float _ratio) { aspectRatio = _ratio; }

    void UpdateThirdPerson(const glm::vec3 &target);
    void SwitchToThirdPerson(const glm::vec3 &target, std::optional<float> distance);
    void SwitchToFirstPerson(const glm::vec3 &position);

    // Returns the view matrix
    glm::mat4 GetViewMatrix() const;

    // Returns the vector that points where the camera is facing
    glm::vec3 getFront(void) const {
        return orientation * glm::vec3(0.0f, 0.0f, -1.0f); // -Z is forward in OpenGL
    }
    // Returns the vector that points at the right of the camera
    glm::vec3 getRight(void) const {
        return orientation * glm::vec3(1.0f, 0.0f, 0.0f); // +X is right
    }

    // Returns the vector that points at the left of the camera
    glm::vec3 getUp(void) const {
        return orientation * glm::vec3(0.0f, 1.0f, 0.0f); // +Y is up
    }

    // Returns the projection matrix
    glm::mat4 GetProjectionMatrix() const;

    // Processes keyboard input with physics
    glm::vec3 ApplyKeyboardPhysics(Camera_Movement direction, float deltaTime, glm::vec3 velocity);

    glm::vec3 ProcessKeyboard(Camera_Movement direction, float deltaTime, float movement_velocity);

    // Process mouse movement input
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // Process mouse scroll event
    void ProcessMouseScroll(float yoffset);

    // Checks if a chunk is visible
    bool isChunkVisible(const AABB &bounds) const {
        const auto planes = extractFrustumPlanes();

        for (const auto &plane : planes) {
            glm::vec3 farthestPoint = bounds.getFarthestPoint(plane.normal);

            float distance = glm::dot(plane.normal, farthestPoint) + plane.distance;
            if (distance < 0.0f) {
                return false; // AABB is fully outside this plane
            }
        }

        return true; // AABB intersects or is inside all planes
    }


    // Camera options
    float MovementSpeed = 2.5f;
    float MouseSensitivity = 0.2f;
    float FOV = 90.0f;
    float aspectRatio = 16.0f / 9.0f;
    float FAR_PLANE = 1000.0f;
    float NEAR_PLANE = 0.01f;
    bool trackMouse;
    // Third-person camera properties
    glm::vec3 Target;           // The player position (focus point)
    float Distance = 5.0f;      // Distance from the camera to the player (third-person only)
    bool isThirdPerson = false; // Toggle between first-person and third-person
  private:
    // Camera attributes
    glm::vec3 Position;
    glm::quat orientation;
    glm::vec3 WorldUp;




    // Frustum planes
    struct FrustumPlane {
        glm::vec3 normal;
        float distance;
    };

    // Extract frustum planes from the view-projection matrix
    std::array<FrustumPlane, 6> extractFrustumPlanes() const;

};
