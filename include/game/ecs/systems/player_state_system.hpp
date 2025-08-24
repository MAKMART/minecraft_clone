#pragma once
#include "../component_manager.hpp"
#include "../components/player_state.hpp"
#include "../components/input.hpp"
#include "../components/velocity.hpp"
#include "../components/player_mode.hpp"
#include "game/player.h"

template<typename... Components>
void player_state_system(ComponentManager<Components...>& cm, Player& player, float dt) {
	cm.template for_each_components<PlayerState, InputComponent, Velocity, PlayerMode>([&](Entity& e, PlayerState& ps, InputComponent& ic, Velocity& vel, PlayerMode& pm){
			glm::vec3 moveDir(0.0f);

			// Build movement vector
			if (ic.forward)  moveDir.z -= 1.0f;
			if (ic.backward) moveDir.z += 1.0f;
			if (ic.left)     moveDir.x -= 1.0f;
			if (ic.right)    moveDir.x += 1.0f;

			if (glm::length(moveDir) > 0.0f)
			moveDir = glm::normalize(moveDir);

			// Update state + velocity
			ps.previous = ps.current;


			// Handle jumping separately (priority over ground states)
			if (player.canJump && ic.jump && ps.current != PlayerMovementState::Flying) {
			vel.value.y = std::sqrt(2 * std::abs(GRAVITY) * player.h);; // maybe don't freaking use std::sqrt() ya know
			ps.current  = PlayerMovementState::Jumping;
			return; // early exit to avoid overwriting jump
			}

			// Movement + state logic
			if (ps.current == PlayerMovementState::Flying && player.canFly) {
				vel.value = moveDir * 10.0f;
			} else if (ic.crouch && player.canCrouch) {
				ps.current = PlayerMovementState::Crouching;
				vel.value  = moveDir * 2.5f;
			} else if (ic.sprint && player.canRun) {
				ps.current = PlayerMovementState::Running;
				vel.value  = moveDir * 7.5f;
			} else if (glm::length(moveDir) > 0.1f && player.canWalk) {
				ps.current = PlayerMovementState::Walking;
				vel.value  = moveDir * 5.0f;
			} else {
				ps.current = PlayerMovementState::Idle;
				vel.value  = glm::vec3(0.0f);
			}
	});
}
