#pragma once
#include "core/defines.hpp"
#include "../component_manager.hpp"
#include "../components/input.hpp"
#include "core/input_manager.hpp"
#include "../components/mouse_input.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

template <typename... Components>
void update_input(ComponentManager<Components...>& cm, const InputManager& input)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	cm.template for_each_component<InputComponent>([&](Entity e, InputComponent& ic) {
		// Reset state
		ic.forward  = input.isHeld(FORWARD_KEY);
		ic.backward = input.isHeld(BACKWARD_KEY);
		ic.left     = input.isHeld(LEFT_KEY);
		ic.right    = input.isHeld(RIGHT_KEY);
		ic.jump     = input.isPressed(JUMP_KEY);
		ic.sprint   = input.isHeld(SPRINT_KEY);
		ic.crouch   = input.isHeld(CROUCH_KEY);

		auto delta           = input.getMouseDelta();
		ic.mouseDelta           = (glm::length(delta) > 0.01f) ? delta : glm::vec2(0.0f);
		ic.is_primary_pressed   = input.isMousePressed(ATTACK_BUTTON);
		ic.is_secondary_pressed = input.isMousePressed(DEFENSE_BUTTON);
		ic.is_ternary_pressed   = input.isMousePressed(GLFW_MOUSE_BUTTON_3);
		if (auto* mi = cm.template get_optional_component<MouseInput>(e)) {
			mi->delta = (glm::length(delta) > 0.01f) ? delta : glm::vec2(0.0f);
			mi->scroll = input.getScroll();
		}
	});
}
