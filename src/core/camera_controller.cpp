#include "core/camera_controller.hpp"
#include "core/logger.hpp"


CameraController::CameraController(const glm::vec3& startPos, const glm::quat& startOrient)
  : currentPosition(startPos),
    currentOrientation(startOrient)
{
    interpolatePosition = currentPosition;
    interpolateOrientation = currentOrientation;
}

CameraController::CameraController()
    : currentPosition(glm::vec3(0.0f)),
      currentOrientation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
{
    interpolatePosition = currentPosition;
    interpolateOrientation = currentOrientation;
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

void CameraController::setPosition(const glm::vec3& pos) {
    if (interpolateEnabled) {
        interpolatePosition = pos;
    } else {
        currentPosition = pos;
        interpolatePosition = pos;
    }
    viewDirty = true;  // position affects view matrix
}
void CameraController::setOrientation(const glm::quat& orient) {
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
    if (isThirdPerson) {
        float yawRad = glm::radians(orbitYaw);
        float pitchRad = glm::radians(orbitPitch);

        glm::vec3 offset;
        offset.x = orbitDistance * std::cos(pitchRad) * std::cos(yawRad);
        offset.y = orbitDistance * std::sin(pitchRad);
        offset.z = orbitDistance * std::cos(pitchRad) * std::sin(yawRad);

        currentPosition = target - offset;
        glm::vec3 front = glm::normalize(target - currentPosition);

        // Recompute orientation from front
        currentOrientation = glm::quatLookAt(front, worldUp);
        viewDirty = true;
    } else if (interpolateEnabled) {
        currentPosition = interpolatePosition.getValue();
        currentOrientation = interpolateOrientation.getValue();
        viewDirty = true;
    }

}
void CameraController::syncYawPitchFromOrientation() {
    glm::vec3 front = glm::normalize(currentOrientation * glm::vec3(0, 0, -1));
    
    float yaw   = glm::degrees(atan2(front.z, front.x));
    float pitch = glm::degrees(asin(glm::clamp(front.y, -1.0f, 1.0f)));

    fpsYaw = orbitYaw = yaw;
    fpsPitch = orbitPitch = pitch;
}

void CameraController::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    const float pitchLimit = 89.0f;

    if (isThirdPerson) {
        orbitYaw += xoffset;
        orbitPitch -= yoffset;

        if (constrainPitch) {
            orbitPitch = glm::clamp(orbitPitch, -pitchLimit, pitchLimit);
        }

        viewDirty = true;
        return;
    }

    // FPS mode
    fpsYaw += xoffset;
    fpsPitch -= yoffset;

    if (constrainPitch)
        fpsPitch = glm::clamp(fpsPitch, -pitchLimit, pitchLimit);

    // Rebuild orientation from yaw and pitch
    glm::vec3 direction;
    direction.x = std::cos(glm::radians(fpsYaw)) * std::cos(glm::radians(fpsPitch));
    direction.y = std::sin(glm::radians(fpsPitch));
    direction.z = std::sin(glm::radians(fpsYaw)) * std::cos(glm::radians(fpsPitch));

    glm::vec3 front = glm::normalize(direction);
    
    currentOrientation = glm::quatLookAt(front, worldUp);
    viewDirty = true;
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
