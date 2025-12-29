#pragma once
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/player_state.hpp"
#include "game/ecs/components/player_mode.hpp"
#include "game/ecs/components/input.hpp"
#include "game/ecs/components/velocity.hpp"
#include "game/ecs/components/collider.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/player.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

// Note: This system must be called after the physics_system
void update_player_state(ECS& ecs, Player& player, float dt)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<PlayerState, InputComponent, Velocity, PlayerMode, Collider>(
	    [&](Entity& e, PlayerState& ps, InputComponent& ic, Velocity& vel, PlayerMode& pm, Collider& col) {
		    // --- Build local movement direction from input ---
		    glm::vec3 inputDir(0.0f);

		    if (ic.forward)
			    inputDir.z -= 1.0f;
		    if (ic.backward)
			    inputDir.z += 1.0f;
		    if (ic.left)
			    inputDir.x -= 1.0f;
		    if (ic.right)
			    inputDir.x += 1.0f;

		    if (glm::length(inputDir) > 0.0f)
			    inputDir = glm::normalize(inputDir);

		    Camera* cam = ecs.get_component<Camera>(player.getCamera());

		    // --- Rotate movement into world-space based on where the player is looking ---
		    glm::vec3 forward = cam->forward;
		    glm::vec3 right   = cam->right;

		    forward.y = 0.0f;
		    if (glm::length(forward) > 0.0f)
			    forward = glm::normalize(forward);

		    glm::vec3 moveDir = (right * inputDir.x) + (forward * inputDir.z);
		    if (glm::length(moveDir) > 0.0f)
			    moveDir = glm::normalize(moveDir);

		    ps.previous = ps.current;

		    // --- Jumping ---
		    if (player.canJump && ic.jump && col.is_on_ground && ps.current != PlayerMovementState::Flying) {
			    vel.value.y      = std::sqrt(2 * std::abs(GRAVITY) * player.h);
			    ps.current       = PlayerMovementState::Jumping;
			    col.is_on_ground = false; // immediately mark as airborne
		    }

		    // --- Transition Jumping → Falling ---
		    if (ps.current == PlayerMovementState::Jumping && vel.value.y <= 0.0f) {
			    ps.current = PlayerMovementState::Falling;
		    }

		    // --- Transition Falling → Idle/Walking/Running when grounded ---
		    if (ps.current == PlayerMovementState::Falling && col.is_on_ground) {
			    if (glm::length(moveDir) > 0.1f && player.canWalk) {
				    ps.current = PlayerMovementState::Walking;
			    } else {
				    ps.current = PlayerMovementState::Idle;
			    }
		    }

		    // Apply horizontal movement
		    glm::vec3 horizontalVel = glm::vec3(moveDir.x, 0.0f, moveDir.z);

		    // Multiply by speed based on state
		    float speed = 0.0f;
		    if (ps.current == PlayerMovementState::Flying && player.canFly) {
			    speed = 10.0f;
		    } else if (ic.crouch && player.canCrouch) {
			    ps.current = PlayerMovementState::Crouching;
			    speed      = 2.5f;
		    } else if (ic.sprint && player.canRun) {
			    ps.current = PlayerMovementState::Running;
			    speed      = 7.5f;
		    } else if (glm::length(moveDir) > 0.1f && player.canWalk) {
			    ps.current = PlayerMovementState::Walking;
			    speed      = 5.0f;
		    } else {
			    ps.current = PlayerMovementState::Idle;
			    speed      = 0.0f;
		    }

		    vel.value.x = moveDir.x * speed;
		    vel.value.z = moveDir.z * speed;
	    });
}
