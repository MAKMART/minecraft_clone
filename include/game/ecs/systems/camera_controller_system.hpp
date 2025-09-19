#pragma once
#include "chunk/chunk.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/ecs/components/camera_controller.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/active_camera.hpp"
#include "game/ecs/components/input.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
#include "core/input_manager.hpp"
#include <glm/gtx/quaternion.hpp>

void update_camera_controller(ECS& ecs, Entity player)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<CameraController, Camera, Transform, ActiveCamera>(
	    [&](Entity e, CameraController& ctrl, Camera& cam, Transform& camTransform, ActiveCamera& ac) {
		    if (ctrl.target.id == Entity{UINT32_MAX}.id)
			    return;

		    auto* targetTransform = ecs.get_component<Transform>(ctrl.target);
		    if (!targetTransform)
			    return;

		    auto* input = ecs.get_component<InputComponent>(player);
		    if (!input)
			    return;

		    // Effective target position = entity position + offset
		    glm::vec3 targetPos = targetTransform->pos + targetTransform->rot * ctrl.offset;

		    // Apply mouse input only if mouseTracking is enabled
		    if (InputManager::get().isMouseTrackingEnabled()) {
			    float smoothing   = 0.19f; // 0 = no smoothing, 1 = very smooth
			    float targetYaw   = ctrl.yaw - input->mouse_delta.x * ctrl.sensitivity;
			    float targetPitch = ctrl.pitch - input->mouse_delta.y * ctrl.sensitivity;

			    ctrl.yaw   = glm::mix(ctrl.yaw, targetYaw, smoothing);
			    ctrl.pitch = glm::mix(ctrl.pitch, targetPitch, smoothing);
			    ctrl.pitch = glm::clamp(ctrl.pitch, -89.0f, 89.0f);
		    }

		    if (ctrl.third_person) {
			    // Compute orbit offset
			    glm::vec3 offset;
			    offset.x         = ctrl.orbit_distance * -1.0f * cos(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			    offset.y         = ctrl.orbit_distance * sin(glm::radians(ctrl.pitch));
			    offset.z         = ctrl.orbit_distance * sin(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			    camTransform.pos = targetPos - offset;
			    // Make camera look at target
			    glm::vec3 dir = glm::normalize(targetPos - camTransform.pos);
			    if (glm::abs(glm::dot(dir, glm::vec3(0, 1, 0))) > 0.999f) {
				    camTransform.rot = glm::quatLookAt(dir, glm::vec3(0, 0, 1));
			    } else {
				    camTransform.rot = glm::quatLookAt(dir, glm::vec3(0, 1, 0));
			    }

		    } else {
			    // First-person
			    glm::quat qPitch = glm::angleAxis(glm::radians(ctrl.pitch), glm::vec3(1, 0, 0));
			    glm::quat qYaw   = glm::angleAxis(glm::radians(ctrl.yaw), glm::vec3(0, 1, 0));
			    camTransform.rot = qYaw * qPitch;
			    camTransform.pos = targetPos;
		    }
		    camTransform.rot = glm::normalize(camTransform.rot);

		    cam.forward = camTransform.rot * glm::vec3(0, 0, -1);
		    cam.right   = camTransform.rot * glm::vec3(1, 0, 0);
		    cam.up      = camTransform.rot * glm::vec3(0, 1, 0);

		    cam.viewMatrix = glm::mat4_cast(glm::conjugate(camTransform.rot)) * glm::translate(glm::mat4(1.0f), -camTransform.pos);

		    cam.projectionMatrix = glm::perspectiveRH_NO(
		        glm::radians(cam.fov),
		        cam.aspect_ratio,
		        cam.far_plane,
		        cam.near_plane);
	    });
}

#if 0
struct FrustumPlane {
		glm::vec4 equation; // ax + by + cz + d = 0

		glm::vec3 normal() const
		{
			return glm::vec3(equation);
		}
		float offset() const
		{
			return equation.w;
		}
	};

	std::array<FrustumPlane, 6> extractFrustumPlanes(const glm::mat4& view, const glm::mat4& projection) const;

std::array<_Camera::FrustumPlane, 6> _Camera::extractFrustumPlanes(const glm::mat4& view, const glm::mat4& proj) const
{
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

#endif
