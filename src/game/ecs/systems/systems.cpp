module;
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
#include <cstdint>
#include <cmath>
#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
module ecs_systems;

import core;
import gl_state;
import glm;
import std;
import ecs_components;
import chunk;
import input_manager;
import aabb;
import logger;

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
			    offset.x         = ctrl.orbit_distance * -std::cos(glm::radians(ctrl.yaw)) * std::cos(glm::radians(ctrl.pitch));
			    offset.y         = ctrl.orbit_distance * std::sin(glm::radians(ctrl.pitch));
			    offset.z         = ctrl.orbit_distance * std::sin(glm::radians(ctrl.yaw)) * std::cos(glm::radians(ctrl.pitch));
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

void chunk_renderer_system(ECS& ecs, ChunkManager& cm, Camera* cam, FrustumVolume& wanted_fv, FramebufferManager& fb)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	ecs.for_each_components<Transform, RenderTarget, ActiveCamera>(
	    [&](Entity e, Transform& ts, RenderTarget& rt, ActiveCamera) {
		const auto& cur_fb = fb.get(e);
		cur_fb.bind_draw();
		GLState::set_viewport(0, 0, cur_fb.width(), cur_fb.height());
		// GLState::get_depth_test() ensures we only clear DEPTH if it's active
		GLbitfield clear_mask = GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		if (GLState::is_depth_test()) {
		clear_mask |= GL_DEPTH_BUFFER_BIT;
		}
		glClear(clear_mask);

		GLState::set_face_culling(true);
		GLState::set_blending(false);
		cm.render_opaque(ts, wanted_fv);

		//GLState::set_face_culling(false); // Water usually needs both sides or special culling
		//GLState::set_blending(true);
		//GLState::set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//cm.render_transparent(ts, wanted_fv);
	    });
}


void input_system(ECS& ecs)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	ecs.for_each_component<InputComponent>([&](Entity e, InputComponent& ic) {
		InputManager& input = InputManager::get();
		// Reset state
		ic.forward  = input.isHeld(FORWARD_KEY);
		ic.backward = input.isHeld(BACKWARD_KEY);
		ic.left     = input.isHeld(LEFT_KEY);
		ic.right    = input.isHeld(RIGHT_KEY);
		ic.jump     = input.isHeld(JUMP_KEY);
		ic.sprint   = input.isHeld(SPRINT_KEY);
		ic.crouch   = input.isHeld(CROUCH_KEY);

		glm::vec2 delta = input.getMouseDelta();
		ic.mouse_delta  = (glm::length(delta) > 0.01f) ? delta : glm::vec2(0.0f);
		ic.scroll_delta = input.getScroll();

		ic.is_primary_pressed   = input.isMousePressed(ATTACK_BUTTON);
		ic.is_secondary_pressed = input.isMousePressed(DEFENSE_BUTTON);
		ic.is_ternary_pressed   = input.isMousePressed(GLFW_MOUSE_BUTTON_3);
	});
}

void movement_intent_system(ECS& ecs, const Camera* cam)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	assert(cam && "Camera was nullptr in movement_intent_system");

	ecs.for_each_components<PlayerState, InputComponent, MovementIntent, MovementConfig>(
	    [&](Entity& e, PlayerState& ps, InputComponent& ic, MovementIntent& intent, MovementConfig& cfg) {
		    // --- Build local movement direction from input ---
		    glm::vec3 input_dir(0.0f);

		    if (ic.forward)	input_dir.z += 1.0f;
		    if (ic.backward)input_dir.z -= 1.0f;
			if (ic.left)	input_dir.x -= 1.0f;
		    if (ic.right)	input_dir.x += 1.0f;

		    if (glm::length(input_dir) > 0.0f)
			    input_dir = glm::normalize(input_dir);

		    // --- Rotate movement into world-space based on where the player is looking ---
		    glm::vec3 forward = cam->forward;
		    glm::vec3 right   = cam->right;

		    forward.y = 0.0f;
		    if (glm::length(forward) > 0.0f)
			    forward = glm::normalize(forward);

			right.y = 0.0f;
			right = glm::normalize(right);


			glm::vec3 wish_dir = right * input_dir.x + forward * input_dir.z;
		    if (glm::length(wish_dir) > 0.0f)
			    wish_dir = glm::normalize(wish_dir);

			// --- Write intent ---
            intent.wish_dir = wish_dir;
            intent.jump     = ic.jump && cfg.can_jump;
            intent.sprint   = ic.sprint && cfg.can_run;
            intent.crouch   = ic.crouch && cfg.can_crouch;

		    ps.previous = ps.current;

			if (glm::length(wish_dir) < 0.1f)
				ps.current = PlayerMovementState::Idle;
			else if (intent.crouch)
				ps.current = PlayerMovementState::Crouching;
			else if (intent.sprint)
				ps.current = PlayerMovementState::Running;
			else
				ps.current = PlayerMovementState::Walking;

			});
}


bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager)
{
#if defined(TRACY_ENABLE)
    ZoneScopedN("isCollidingAt");
#endif
    const AABB box = col.getBoundingBoxAt(pos);

    // Integer bounds (inclusive)
    const glm::ivec3 minBlock = glm::floor(box.min);
    const glm::ivec3 maxBlock = glm::floor(box.max - glm::vec3(1e-4f)); // Avoid off-by-one on exact edges

	for (int y = minBlock.y; y <= maxBlock.y; ++y) {
        for (int x = minBlock.x; x <= maxBlock.x; ++x) {
            for (int z = minBlock.z; z <= maxBlock.z; ++z) {
                const glm::ivec3 blockWorldPos(x, y, z);

                Chunk* chunk = chunkManager.getChunk(blockWorldPos);
                if (!chunk)
                    continue; // Treat missing chunks as air (or return true for "void" if preferred)

                const glm::ivec3 local = world_to_local(blockWorldPos);
                const Block::blocks& type = chunk->get_block_type(local.x, local.y, local.z);

                if (type != Block::blocks::AIR &&
                    type != Block::blocks::WATER &&
                    type != Block::blocks::LAVA) {
                    return true;
                }
            }
        }
    }
    return false;
}

void movement_physics_system(ECS& ecs, ChunkManager& chunkManager, float dt)
{
#if defined(TRACY_ENABLE)
    ZoneScopedN("movement_physics_system");
#endif

    ecs.for_each_components<MovementIntent, Velocity, Collider, PlayerState, MovementConfig, Transform>(
        [&](Entity e, MovementIntent& intent, Velocity& vel, Collider& col, PlayerState& ps, MovementConfig& cfg, Transform& trans) {
#if defined(TRACY_ENABLE)
            ZoneScopedN("Player Physics Tick");
#endif

            // --- Apply input to velocity ---
            float speed = 0.0f;
            switch (ps.current) {
                case PlayerMovementState::Walking:   speed = cfg.walk_speed;   break;
                case PlayerMovementState::Running:   speed = cfg.run_speed;   break;
                case PlayerMovementState::Crouching: speed = cfg.crouch_speed; break;
                case PlayerMovementState::Flying:    speed = cfg.fly_speed;    break;
                default:                             speed = 0.0f;             break;
            }

            vel.value.x = intent.wish_dir.x * speed;
            vel.value.z = intent.wish_dir.z * speed;

            if (ps.current == PlayerMovementState::Flying) {
                vel.value.y = intent.wish_dir.y * cfg.fly_speed;
            }

            // --- Jumping ---
            if (cfg.can_jump && intent.jump && col.is_on_ground && !cfg.can_fly) {
                vel.value.y = std::sqrt(2.0f * GRAVITY * cfg.jump_height);
                ps.current = PlayerMovementState::Jumping;
                col.is_on_ground = false;
            }

            // --- Gravity ---
            if (!col.is_on_ground && !cfg.can_fly) {
                vel.value.y -= GRAVITY * dt;
            }

            // --- State transitions ---
            if (ps.current == PlayerMovementState::Jumping && vel.value.y <= 0.0f) {
                ps.current = PlayerMovementState::Falling;
            }
            if (ps.current == PlayerMovementState::Falling && col.is_on_ground) {
                ps.current = ((intent.wish_dir.x * intent.wish_dir.x + intent.wish_dir.y * intent.wish_dir.y + intent.wish_dir.z * intent.wish_dir.z) > 0.01f && cfg.can_walk)
                    ? PlayerMovementState::Walking
                    : PlayerMovementState::Idle;
            }

            // --- Early out if stationary ---
            if (glm::all(glm::epsilonEqual(vel.value, glm::vec3(0.0f), 1e-4f))) {
                col.is_on_ground = isCollidingAt(trans.pos - glm::vec3(0.0f, 0.05f, 0.0f), col, chunkManager);
                col.aabb = col.getBoundingBoxAt(trans.pos);
                return;
            }

            const glm::vec3 oldPos = trans.pos;
            glm::vec3 newPos = trans.pos + vel.value * dt;
            col.is_on_ground = false;

            // --- Axis-by-axis sweep (Y → X → Z) ---
            // Y-axis
            glm::vec3 testPos = { oldPos.x, newPos.y, oldPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.y = testPos.y;
            } else {
                trans.pos.y = oldPos.y;
                if (vel.value.y < 0.0f) {
                    vel.value.y = 0.0f;
                    col.is_on_ground = true;
                } else if (vel.value.y > 0.0f) {
                    vel.value.y = 0.0f;
                }
            }

            // X-axis
            testPos = { newPos.x, trans.pos.y, oldPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.x = testPos.x;
            } else {
                trans.pos.x = oldPos.x;
                vel.value.x = 0.0f;
            }

            // Z-axis
            testPos = { trans.pos.x, trans.pos.y, newPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.z = testPos.z;
            } else {
                trans.pos.z = oldPos.z;
                vel.value.z = 0.0f;
            }

            // --- Final ground check (small downward ray) ---
            if (!col.is_on_ground) {
                col.is_on_ground = isCollidingAt(trans.pos - glm::vec3(0.0f, 0.05f, 0.0f), col, chunkManager);
            }

            // Sync AABB once at the end
            col.aabb = col.getBoundingBoxAt(trans.pos);
        });
}

void camera_system(ECS& ecs)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<Camera, Transform, ActiveCamera>(
	    [&](Entity e, Camera& cam, Transform& transform, ActiveCamera&) {

		
		transform.rot = glm::normalize(transform.rot);

		cam.forward = transform.rot * glm::vec3(0, 0, -1);
		cam.right   = transform.rot * glm::vec3(1, 0, 0);
		cam.up      = transform.rot * glm::vec3(0, 1, 0);

		cam.viewMatrix =
		glm::mat4_cast(glm::conjugate(transform.rot)) *
		glm::translate(glm::mat4(1.0f), -transform.pos);

		// Note that we use RH_ZO because we use reverse-Z depth, if you want to change to forward-Z just change to RH_NO and switch the far with the near plane and vice-versa
		    cam.projectionMatrix = glm::perspectiveRH_ZO(
		        glm::radians(cam.fov),
		        cam.aspect_ratio,
		        cam.far_plane,
		        cam.near_plane);
	    });
}

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
            if(ic.jump)     transform.pos += cam.up * speed * dt;
            if(ic.crouch)   transform.pos -= cam.up * speed * dt;

            // Mouse look
			if (InputManager::get().isMouseTrackingEnabled()) {
			glm::vec2 delta = ic.mouse_delta;
			dc.yaw   += -delta.x * dc.sensitivity;
			dc.pitch += -delta.y * dc.sensitivity;
			dc.pitch = glm::clamp(dc.pitch, -89.0f, 89.0f);
			glm::quat qyaw   = glm::angleAxis(glm::radians(dc.yaw),   glm::vec3(0, 1, 0));
			glm::quat qpitch = glm::angleAxis(glm::radians(dc.pitch), glm::vec3(1, 0, 0));

			transform.rot = glm::normalize(qyaw * qpitch);
			}

			ic.mouse_delta = glm::vec2(0.0f);
        });
}

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

void camera_temporal_system(ECS& ecs)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<Camera, CameraTemporal, ActiveCamera>(
	    [&](Entity e, Camera& cam, CameraTemporal& temp, ActiveCamera&) {

		glm::mat4 curr_view_proj = cam.projectionMatrix * cam.viewMatrix;

		if (temp.first_frame) {
		temp.prev_view_proj = curr_view_proj;
		temp.first_frame = false;
		return;
		}
		// Advance time
		temp.prev_view_proj = curr_view_proj;
	    });
}
