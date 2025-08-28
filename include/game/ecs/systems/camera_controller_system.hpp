#pragma once
#include "../component_manager.hpp"
#include "../components/camera.hpp"
#include "../components/camera_controller.hpp"
#include "../components/transform.hpp"
#include "../components/active_camera.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/tracy.hpp>
#endif


template <typename... Components>
void update_camera_controller(ComponentManager<Components...>& cm)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	cm.template for_each_components<CameraController, Camera, Transform>(
			[&](Entity e, CameraController& ctrl, Camera& cam, Transform& camTransform) {

			if (auto* ac = cm.template get_optional_component<ActiveCamera>(e)) {

			if (ctrl.target.id == Entity{UINT32_MAX}.id) return;

			auto* targetTransform = cm.template get_component<Transform>(ctrl.target);
			if (!targetTransform) return;

			if (ctrl.third_person) {
			glm::vec3 offset;
			offset.x = ctrl.orbit_distance * cos(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			offset.y = ctrl.orbit_distance * sin(glm::radians(ctrl.pitch));
			offset.z = ctrl.orbit_distance * sin(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));

			camTransform.pos = targetTransform->pos - offset;
			cam.viewMatrix   = glm::lookAt(camTransform.pos, targetTransform->pos, {0,1,0});
			} else {
			camTransform.pos = targetTransform->pos;

			glm::vec3 front;
			front.x = cos(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			front.y = sin(glm::radians(ctrl.pitch));
			front.z = sin(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));

			cam.forward = glm::normalize(front);
			cam.right   = glm::normalize(glm::cross(cam.forward, glm::vec3(0,1,0)));
			cam.up      = glm::normalize(glm::cross(cam.right, cam.forward));

			cam.viewMatrix = glm::lookAt(camTransform.pos, camTransform.pos + cam.forward, cam.up);
			}

			cam.projectionMatrix = glm::perspective(
					glm::radians(cam.fov),
					cam.aspect_ratio,
					cam.near_plane,
					cam.far_plane
					);
			}
			}
	);
}

