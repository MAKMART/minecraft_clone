#pragma once
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

template <typename... Components>
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

		    // Apply mouse input only if mouseTracking is enabled
		    if (InputManager::get().isMouseTrackingEnabled()) {
			    float sensitivity = 0.2f;
			    ctrl.yaw += input->mouse_delta.x * sensitivity;
			    ctrl.pitch += -1.0f * input->mouse_delta.y * sensitivity;
			    ctrl.pitch = glm::clamp(ctrl.pitch, -89.0f, 89.0f);

			    float smoothing = 0.15f; // 0 = no smoothing, 1 = very smooth
			    ctrl.yaw        = glm::mix(ctrl.yaw, ctrl.yaw + input->mouse_delta.x * sensitivity, smoothing);
			    ctrl.pitch      = glm::mix(ctrl.pitch, ctrl.pitch + input->mouse_delta.y * sensitivity, smoothing);
		    }

		    if (ctrl.third_person) {
			    glm::vec3 offset;
			    offset.x = ctrl.orbit_distance * cos(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			    offset.y = ctrl.orbit_distance * sin(glm::radians(ctrl.pitch));
			    offset.z = ctrl.orbit_distance * sin(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));

			    camTransform.pos = targetTransform->pos - offset;
			    cam.viewMatrix   = glm::lookAt(camTransform.pos, targetTransform->pos, {0, 1, 0});
		    } else {
			    camTransform.pos = targetTransform->pos;

			    glm::vec3 front;
			    front.x = cos(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			    front.y = sin(glm::radians(ctrl.pitch));
			    front.z = sin(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));

			    cam.forward = glm::normalize(front);
			    cam.right   = glm::normalize(glm::cross(cam.forward, glm::vec3(0, 1, 0)));
			    cam.up      = glm::normalize(glm::cross(cam.right, cam.forward));

			    cam.viewMatrix = glm::lookAt(camTransform.pos, camTransform.pos + cam.forward, cam.up);
		    }

		    cam.projectionMatrix = glm::perspective(
		        glm::radians(cam.fov),
		        cam.aspect_ratio,
		        cam.near_plane,
		        cam.far_plane);
	    });
}
