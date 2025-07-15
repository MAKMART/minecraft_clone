#pragma once
#include "Camera.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Animation.hpp"
#include "AABB.h"


enum class Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
};


class CameraController {
public:

    CameraController(const glm::vec3& startPos, const glm::quat& startOrient);

    CameraController();

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    const glm::vec3& getCurrentPosition() const { return currentPosition; }
    const glm::quat& getCurrentOrientation() const { return currentOrientation; }
    const Camera& getCamera() const { return camera; }
    float getFov() const { return fov; }
    float getAspectRatio() const { return aspectRatio; }
    float getNearPlane() const { return nearPlane; }
    float getFarPlane() const { return farPlane; }
    float getSensitivity() const { return sensitivity; }
    glm::vec3 getFront() const;
    glm::vec3 getRight() const;
    glm::vec3 getUp() const;
    const glm::vec3& getThirdPersonOffset() const { return thirdPersonOffset; }
    bool isThirdPersonMode() const { return isThirdPerson; }
    bool isInterpolationEnabled() const;
    bool isAABBVisible(const AABB &box) const;


    void setThirdPersonDistance(float d) {
        thirdPersonDistance = d;
        thirdPersonOffset = glm::vec3(0.0f, 2.0f, -thirdPersonDistance);
    }
    void setThirdPersonOffset(const glm::vec3& offset) {
        thirdPersonOffset = offset;
        viewDirty = true;
    }

    void setFov(float newFov) {
        if (fov != newFov) {
            fov = newFov;
            projectionDirty = true;
        }
    }
    void setAspectRatio(float newAspect) {
        if (aspectRatio != newAspect) {
            aspectRatio = newAspect;
            projectionDirty = true;
        }
    }
    void setNearPlane(float newNear) {
        if (nearPlane != newNear) {
            nearPlane = newNear;
            projectionDirty = true;
        }
    }
    void setFarPlane(float newFar) {
        if (farPlane != newFar) {
            farPlane = newFar;
            projectionDirty = true;
        }
    }
    void setSensitivity(float newSens) {
        if (sensitivity != newSens) {
            sensitivity = newSens;
        }
    }
    void setThirdPersonMode(bool enabled) {
        if (isThirdPerson != enabled) {
            isThirdPerson = enabled;
            viewDirty = true;  // Because view depends on mode
        }
    }
    void toggleThirdPersonMode() {
        setThirdPersonMode(!isThirdPerson);
    }
    void setInterpolationEnabled(bool enabled);
    void setInterpolationDuration(float seconds);
    void setTargetPosition(const glm::vec3& pos);
    void setTargetOrientation(const glm::quat& orient);

    void snapToTarget();

    void update(float deltaTime);

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    void updateFrustumPlanes() const;


private:
    Camera camera;

    Interpolated<glm::vec3> interpolatePosition;
    Interpolated<glm::quat> interpolateOrientation;

    bool interpolateEnabled;

    mutable bool viewDirty = true;
    mutable bool projectionDirty = true;
    mutable bool frustumDirty = true;

    mutable std::array<Camera::FrustumPlane, 6> frustumPlanes;

    mutable glm::mat4 cachedViewMatrix;
    mutable glm::mat4 cachedProjectionMatrix;

    glm::vec3 currentPosition;
    glm::quat currentOrientation;

    // Projection params
    float fov = 90.0f;  // DEG
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.01f;
    float farPlane = 1000.0f;
    float sensitivity = 0.1f;


    float thirdPersonDistance = 6.0f;
    glm::vec3 thirdPersonOffset = glm::vec3(0.0f, 2.0f, -thirdPersonDistance);
    bool isThirdPerson;
};
