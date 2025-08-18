#include "core/Camera.hpp"
#include <glm/common.hpp>

glm::mat4 Camera::computeViewMatrix(const glm::vec3& cameraPos, const glm::quat& cameraOrientation) const {
    return glm::mat4_cast(glm::conjugate(cameraOrientation)) * glm::translate(glm::mat4(1.0f), -cameraPos);
}

glm::mat4 Camera::computeProjectionMatrix(float fovDegrees, float aspect, float nearPlane, float farPlane) const {
    return glm::perspective(glm::radians(fovDegrees), aspect, nearPlane, farPlane);
}

std::array<Camera::FrustumPlane, 6> Camera::extractFrustumPlanes(const glm::mat4 &view, const glm::mat4 &proj) const {
    glm::mat4 VP = proj * view;

    std::array<FrustumPlane, 6> frustum;

    // Left
    frustum[0].equation = glm::vec4(
        VP[0][3] + VP[0][0],
        VP[1][3] + VP[1][0],
        VP[2][3] + VP[2][0],
        VP[3][3] + VP[3][0]);

    // Right
    frustum[1].equation = glm::vec4(
        VP[0][3] - VP[0][0],
        VP[1][3] - VP[1][0],
        VP[2][3] - VP[2][0],
        VP[3][3] - VP[3][0]);

    // Bottom
    frustum[2].equation = glm::vec4(
        VP[0][3] + VP[0][1],
        VP[1][3] + VP[1][1],
        VP[2][3] + VP[2][1],
        VP[3][3] + VP[3][1]);

    // Top
    frustum[3].equation = glm::vec4(
        VP[0][3] - VP[0][1],
        VP[1][3] - VP[1][1],
        VP[2][3] - VP[2][1],
        VP[3][3] - VP[3][1]);

    // Near
    frustum[4].equation = glm::vec4(
        VP[0][3] + VP[0][2],
        VP[1][3] + VP[1][2],
        VP[2][3] + VP[2][2],
        VP[3][3] + VP[3][2]);

    // Far
    frustum[5].equation = glm::vec4(
        VP[0][3] - VP[0][2],
        VP[1][3] - VP[1][2],
        VP[2][3] - VP[2][2],
        VP[3][3] - VP[3][2]);

    // Normalize the planes
    for (auto& plane : frustum) {
        float length = glm::length(glm::vec3(plane.equation));
        if (length > 0.0f) {
            plane.equation /= length;
        }
    }

    return frustum;
}
