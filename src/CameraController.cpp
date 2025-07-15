#include "CameraController.hpp"


CameraController::CameraController(const glm::vec3& startPos, const glm::quat& startOrient)
  : interpolateEnabled(false),
    currentPosition(startPos),
    currentOrientation(startOrient),
    isThirdPerson(false)
{
    interpolatePosition = currentPosition;
    interpolateOrientation = currentOrientation;
    setInterpolationDuration(0.5f);
}

CameraController::CameraController()
    : interpolateEnabled(false),
      currentPosition(glm::vec3(0.0f)),
      currentOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
      isThirdPerson(false)
{
    // Initialize interpolation targets with current values
    interpolatePosition = currentPosition;
    interpolateOrientation = currentOrientation;

    // Default interpolation duration (speed inverse)
    setInterpolationDuration(0.5f); // half a second transition
}

glm::mat4 CameraController::getViewMatrix() const {
    if (viewDirty) {
        cachedViewMatrix = camera.computeViewMatrix(currentPosition, currentOrientation);
        viewDirty = false;
    }
    return cachedViewMatrix;
}
glm::mat4 CameraController::getProjectionMatrix() const {
    if (projectionDirty) {
        cachedProjectionMatrix = camera.computeProjectionMatrix(fov, aspectRatio, nearPlane, farPlane);
        projectionDirty = false;
    }
    return cachedProjectionMatrix;
}
glm::vec3 CameraController::getFront() const {
    return glm::normalize(currentOrientation * glm::vec3(0,0,-1));
}

glm::vec3 CameraController::getRight() const {
    return glm::normalize(currentOrientation * glm::vec3(1,0,0));
}

glm::vec3 CameraController::getUp() const {
    return glm::normalize(currentOrientation * glm::vec3(0,1,0));
}


bool CameraController::isInterpolationEnabled() const {
    return interpolateEnabled;
}
bool CameraController::isAABBVisible(const AABB& box) const {
    if (frustumDirty) {
        updateFrustumPlanes();
    }

    // For each plane, check the box
    for (const auto& plane : frustumPlanes) {
        // Compute the positive vertex for the plane normal
        glm::vec3 positiveVertex = box.min;

        if (plane.normal().x >= 0) positiveVertex.x = box.max.x;
        if (plane.normal().y >= 0) positiveVertex.y = box.max.y;
        if (plane.normal().z >= 0) positiveVertex.z = box.max.z;

        // If positive vertex is outside plane, the box is outside
        if (glm::dot(plane.normal(), positiveVertex) + plane.offset() < 0) {
            return false; // Outside this plane
        }
    }

    return true; // Inside or intersects all planes
}

void CameraController::setInterpolationEnabled(bool enabled) {
    interpolateEnabled = enabled;
    if (!enabled) {
        // Snap current to target immediately if disabling interpolation
        currentPosition = interpolatePosition.getValue();
        currentOrientation = interpolateOrientation.getValue();
    }
}

void CameraController::setInterpolationDuration(float seconds) {
    // Speed is inverse of duration
    interpolatePosition.setDuration(seconds);
    interpolateOrientation.setDuration(seconds);
}

void CameraController::setTargetPosition(const glm::vec3& pos) {
    if (interpolateEnabled) {
        interpolatePosition = pos;
    } else {
        currentPosition = pos;
        interpolatePosition = pos;
    }
    viewDirty = true;  // position affects view matrix
}

void CameraController::setTargetOrientation(const glm::quat& orient) {
    if (interpolateEnabled) {
        interpolateOrientation = orient;
    } else {
        currentOrientation = orient;
        interpolateOrientation = orient;
    }
    viewDirty = true;  // orientation affects view matrix
}
void CameraController::snapToTarget() {
    currentPosition = interpolatePosition.getValue();
    currentOrientation = interpolateOrientation.getValue();
}

void CameraController::update(float deltaTime) {
    if (interpolateEnabled) {
        currentPosition = interpolatePosition.getValue();
        currentOrientation = interpolateOrientation.getValue();
        viewDirty = true;
    }
    viewDirty = true;
    // No else, currentPosition/orientation already set for no interpolation
}
void CameraController::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    // Update currentOrientation based on input offsets (you probably have pitch/yaw tracking)
    // For example, update Euler angles, then recalc currentOrientation quaternion
    
    // Here's a typical implementation idea:
    static float yaw = -90.0f;  // Initialize facing forward -Z
    static float pitch = 0.0f;

    yaw += xoffset * sensitivity;
    pitch += yoffset * sensitivity;

    // Clamp pitch
    if (constrainPitch) {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    // Calculate new orientation quaternion from yaw/pitch (roll=0)
    glm::vec3 front;
    front.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front.y = std::sin(glm::radians(pitch));
    front.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(front);

    // Construct quaternion facing that direction: 
    // Since you want orientation quaternion, you can do lookAt from world forward
    // Or use glm::quatLookAt or equivalent:

    currentOrientation = glm::quatLookAt(front, glm::vec3(0, 1, 0));
    viewDirty = true;  // Mark view matrix dirty for recalculation
}

void CameraController::processMouseScroll(float yoffset) {
    // Adjust FOV for zoom effect
    fov -= yoffset;
    if (fov < 10.0f) fov = 10.0f;
    if (fov > 160.0f) fov = 160.0f;

    projectionDirty = true;  // Mark projection matrix dirty for recalculation
}
void CameraController::updateFrustumPlanes() const {
 
    frustumPlanes = camera.extractFrustumPlanes(cachedViewMatrix, cachedProjectionMatrix);

    frustumDirty = false;
}
