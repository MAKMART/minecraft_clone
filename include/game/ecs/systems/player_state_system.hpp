#pragma once
#include "../component_manager.hpp"
#include "../components/player_state.hpp"
#include "../components/input.hpp"
#include "../components/velocity.hpp"
#include "../components/player_mode.hpp"
#include "game/player.hpp"

template<typename... Components>
void player_state_system(ComponentManager<Components...>& cm, Player& player, float dt) {
    cm.template for_each_components<PlayerState, InputComponent, Velocity, PlayerMode>(
        [&](Entity& e, PlayerState& ps, InputComponent& ic, Velocity& vel, PlayerMode& pm) {
            
            // --- Build local movement direction from input ---
            glm::vec3 inputDir(0.0f);

            if (ic.forward)  inputDir.z += 1.0f;
            if (ic.backward) inputDir.z -= 1.0f;
            if (ic.left)     inputDir.x -= 1.0f;
            if (ic.right)    inputDir.x += 1.0f;

            if (glm::length(inputDir) > 0.0f)
                inputDir = glm::normalize(inputDir);

            // --- Rotate movement into world-space based on where the player is looking ---
            // Assume tr.rotation is a quaternion (glm::quat) or matrix
            glm::vec3 forward = player.getCameraFront();
            glm::vec3 right   = player.getCameraRight();

            glm::vec3 moveDir = (right * inputDir.x) + (forward * inputDir.z);
            if (glm::length(moveDir) > 0.0f)
                moveDir = glm::normalize(moveDir);

            // --- Preserve vertical velocity for jumping/falling ---
            float verticalVel = vel.value.y;

            ps.previous = ps.current;

            // --- Jumping ---
            if (player.canJump && ic.jump && ps.current != PlayerMovementState::Flying) {
                verticalVel = std::sqrt(2 * std::abs(GRAVITY) * player.h);
                ps.current  = PlayerMovementState::Jumping;
            }

	    // Apply horizontal movement
	    glm::vec3 horizontalVel = glm::vec3(moveDir.x, 0.0f, moveDir.z);

	    // Multiply by speed based on state
	    float speed = 0.0f;
	    if (ps.current == PlayerMovementState::Flying && player.canFly) {
		    speed = 10.0f;
	    } else if (ic.crouch && player.canCrouch) {
		    ps.current = PlayerMovementState::Crouching;
		    speed = 2.5f;
	    } else if (ic.sprint && player.canRun) {
		    ps.current = PlayerMovementState::Running;
		    speed = 7.5f;
	    } else if (glm::length(moveDir) > 0.1f && player.canWalk) {
		    ps.current = PlayerMovementState::Walking;
		    speed = 5.0f;
	    } else {
		    ps.current = PlayerMovementState::Idle;
		    speed = 0.0f;
	    }

	    // Combine horizontal and vertical velocities
	    vel.value = horizontalVel * speed;
	    vel.value.y = verticalVel;
    });
}

