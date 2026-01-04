#pragma once

#include "game/ecs/ecs.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "game/ecs/components/camera.hpp"
#include <glm/common.hpp>

void extractFrustumPlanes(const glm::mat4& VP, std::array<FrustumVolume::FrustumPlane,6>& planes) {
    planes[0].equation = glm::vec4(VP[0][3] + VP[0][0], VP[1][3] + VP[1][0], VP[2][3] + VP[2][0], VP[3][3] + VP[3][0]);
    planes[1].equation = glm::vec4(VP[0][3] - VP[0][0], VP[1][3] - VP[1][0], VP[2][3] - VP[2][0], VP[3][3] - VP[3][0]);
    planes[2].equation = glm::vec4(VP[0][3] + VP[0][1], VP[1][3] + VP[1][1], VP[2][3] + VP[2][1], VP[3][3] + VP[3][1]);
    planes[3].equation = glm::vec4(VP[0][3] - VP[0][1], VP[1][3] - VP[1][1], VP[2][3] - VP[2][1], VP[3][3] - VP[3][1]);
    planes[4].equation = glm::vec4(VP[0][3] + VP[0][2], VP[1][3] + VP[1][2], VP[2][3] + VP[2][2], VP[3][3] + VP[3][2]);
    planes[5].equation = glm::vec4(VP[0][3] - VP[0][2], VP[1][3] - VP[1][2], VP[2][3] - VP[2][2], VP[3][3] - VP[3][2]);

    for (auto& plane : planes) {
        float len = glm::length(glm::vec3(plane.equation));
        if (len > 0.0f) plane.equation /= len;
    }
}

void frustum_volume_system(ECS& ecs)
{

	ecs.for_each_components<Camera, FrustumVolume>(
			[&](Entity e, Camera& cam, FrustumVolume& fv) {
			extractFrustumPlanes(cam.projectionMatrix * cam.viewMatrix, fv.planes);
			});

}
