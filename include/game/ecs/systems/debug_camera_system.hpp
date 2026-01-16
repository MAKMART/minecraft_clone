#pragma once
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/ecs/components/input.hpp"
#include "game/ecs/components/debug_camera.hpp"
#include "game/ecs/components/debug_camera_controller.hpp"
#include "core/input_manager.hpp"
#include <glm/common.hpp>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif



void debug_camera_system(ECS& ecs, float dt)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
    ecs.for_each_components<Camera, Transform, InputComponent, DebugCameraController, DebugCamera>(
        [&](Entity e, Camera& cam, Transform& transform, InputComponent& ic, DebugCameraController& dc, DebugCamera&) {
            float speed = 10.0f; // units per second
			if (ic.sprint)
				speed = 100.0f;


            // Movement
            if(ic.forward)  transform.pos += cam.forward * speed * dt;
            if(ic.backward) transform.pos -= cam.forward * speed * dt;
            if(ic.left)     transform.pos -= cam.right * speed * dt;
            if(ic.right)    transform.pos += cam.right * speed * dt;
            if(InputManager::get().isHeld(GLFW_KEY_SPACE))     transform.pos += cam.up * speed * dt;
            if(ic.crouch)   transform.pos -= cam.up * speed * dt;

            // Mouse look
            glm::vec2 delta = ic.mouse_delta;
			dc.yaw   += -delta.x * dc.sensitivity;
			dc.pitch += -delta.y * dc.sensitivity;
			dc.pitch = glm::clamp(dc.pitch, -89.0f, 89.0f);
			glm::quat qyaw   = glm::angleAxis(glm::radians(dc.yaw),   glm::vec3(0, 1, 0));
			glm::quat qpitch = glm::angleAxis(glm::radians(dc.pitch), glm::vec3(1, 0, 0));

			transform.rot = glm::normalize(qyaw * qpitch);

            ic.mouse_delta = glm::vec2(0.0f);
        });
}
