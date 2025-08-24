#pragma once
#include "core/defines.hpp"
#include "../component_manager.hpp"
#include "../components/input.hpp"
#include "core/input_manager.hpp"


template<typename... Components>
void update_input(ComponentManager<Components...>& cm, const InputManager& input) {
	cm.template for_each_component<InputComponent>([&](Entity e, InputComponent& ic) {



			// Reset state
			ic.forward  = input.isHeld(FORWARD_KEY);
			ic.backward = input.isHeld(BACKWARD_KEY);
			ic.left     = input.isHeld(LEFT_KEY);
			ic.right    = input.isHeld(RIGHT_KEY);
			ic.jump     = input.isPressed(JUMP_KEY);
			ic.sprint   = input.isHeld(SPRINT_KEY);
			ic.crouch   = input.isHeld(CROUCH_KEY);

			auto [dx, dy] = input.getMouseDelta();
			ic.mouseDelta = (glm::length(glm::vec2(dx, dy)) > 0.001f)
			? glm::vec2(dx, dy)
			: glm::vec2(0.0f);



			});
}
