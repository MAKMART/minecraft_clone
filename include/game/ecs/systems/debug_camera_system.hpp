#pragma once
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/ecs/components/input.hpp"
#include <glm/common.hpp>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif



void debug_camera_system(ECS& ecs, float dt)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
    ecs.for_each_components<Camera, Transform, InputComponent>(
        [&](Entity e, Camera& cam, Transform& transform, InputComponent& ic) {
            float speed = 10.0f; // units per second
			if (ic.sprint)
				speed = 100.0f;


            // Movement
            if(ic.forward)  transform.pos += cam.forward * speed * dt;
            if(ic.backward) transform.pos -= cam.forward * speed * dt;
            if(ic.left)     transform.pos -= cam.right * speed * dt;
            if(ic.right)    transform.pos += cam.right * speed * dt;
            if(ic.jump)     transform.pos += cam.up * speed * dt;
            if(ic.crouch)   transform.pos -= cam.up * speed * dt;

            // Mouse look
            float sensitivity = 0.002f;
            glm::vec2 delta = ic.mouse_delta;
            glm::quat yaw   = glm::angleAxis(-delta.x * sensitivity, glm::vec3(0,1,0));
            glm::quat pitch = glm::angleAxis(-delta.y * sensitivity, glm::vec3(1,0,0));
            transform.rot   = glm::normalize(yaw * transform.rot * pitch);

            ic.mouse_delta = glm::vec2(0.0f);
        });
}
