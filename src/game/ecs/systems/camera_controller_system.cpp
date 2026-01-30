#include "game/ecs/systems/camera_controller_system.hpp"
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

void camera_controller_system(ECS& ecs, Entity player)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<CameraController, Camera, Transform, ActiveCamera>(
	    [&](Entity e, CameraController& ctrl, Camera& cam, Transform& camTransform, ActiveCamera&) {
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

			    //ctrl.yaw   = glm::mix(ctrl.yaw, targetYaw, smoothing);
			    //ctrl.pitch = glm::mix(ctrl.pitch, targetPitch, smoothing);
				ctrl.yaw = targetYaw;
				ctrl.pitch = targetPitch;
			    ctrl.pitch = glm::clamp(ctrl.pitch, -89.0f, 89.0f);
				camTransform.rot   = glm::normalize(targetYaw * camTransform.rot * targetPitch);
		    }

		    if (ctrl.third_person) {
			    // Compute orbit offset
			    glm::vec3 offset;
			    offset.x         = ctrl.orbit_distance * -cos(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			    offset.y         = ctrl.orbit_distance * sin(glm::radians(ctrl.pitch));
			    offset.z         = ctrl.orbit_distance * sin(glm::radians(ctrl.yaw)) * cos(glm::radians(ctrl.pitch));
			    camTransform.pos = targetPos - offset;
			    // Make camera look at target
			    glm::vec3 dir = glm::normalize(targetPos - camTransform.pos);
			    camTransform.rot = glm::quatLookAt(dir, glm::vec3(0, 1, 0));
		    } else {
			    // First-person
			    glm::quat qPitch = glm::angleAxis(glm::radians(ctrl.pitch), glm::vec3(1, 0, 0));
			    glm::quat qYaw   = glm::angleAxis(glm::radians(ctrl.yaw), glm::vec3(0, 1, 0));
			    camTransform.rot = qYaw * qPitch;
			    camTransform.pos = targetPos;
		    }
		    camTransform.rot = glm::normalize(camTransform.rot);
	    });
}
