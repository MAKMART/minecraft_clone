#pragma once
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/player_state.hpp"
#include "game/ecs/components/player_mode.hpp"
#include "game/ecs/components/input.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/ecs/components/movement_intent.hpp"
#include "game/ecs/components/movement_config.hpp"
#include <glm/common.hpp>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

void movement_intent_system(ECS& ecs, const Entity& active_cam)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	Camera* cam = ecs.get_component<Camera>(active_cam);
	if (!cam)
		return;

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
