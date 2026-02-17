#include "core/input_manager.hpp"

// Remember to call this at the end of whatever input pooling that you do trough isPressed/isHeld etc
void InputManager::update()
{
	_mouseDelta                = glm::vec2(0.0f); // reset after each frame
	_scroll                    = glm::vec2(0.0f);
	for (auto& key : _keyStates) {
		if (key == KeyState::PRESSED)  key = KeyState::DOWN;
		if (key == KeyState::RELEASED) key = KeyState::UP;
	}
	for (auto& btn : _mouseButtonStates) {
		if (btn == KeyState::PRESSED)  btn = KeyState::DOWN;
		if (btn == KeyState::RELEASED) btn = KeyState::UP;
	}
}
void InputManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx || key < 0 || key >= MAX_KEYS)
		return;
	InputManager& im = InputManager::get();

	if (action == GLFW_PRESS)
		im._keyStates[key] = KeyState::PRESSED;
	else if (action == GLFW_RELEASE)
		im._keyStates[key] = KeyState::RELEASED;
}

void InputManager::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx || button < 0 || button >= MAX_MOUSE_BUTTONS)
		return;
	InputManager& im = InputManager::get();

	if (action == GLFW_PRESS)
		im._mouseButtonStates[button] = KeyState::PRESSED;
	else if (action == GLFW_RELEASE)
		im._mouseButtonStates[button] = KeyState::RELEASED;
}

void InputManager::cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx)
		return;
	InputManager& im = InputManager::get();

	glm::vec2 newMouse(static_cast<float>(xpos), static_cast<float>(ypos));
	im._mouseDelta += newMouse - im._lastMouse;
	im._lastMouse = newMouse;

	if (im.invertYAxis)
		im._mouseDelta.y *= -1.0f;
	if (im.invertXAxis)
		im._mouseDelta.x *= -1.0f;
}

void InputManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx)
		return;
	InputManager& im = InputManager::get();

	im._scroll += glm::vec2(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

void InputManager::window_focus_callback(GLFWwindow* window, int focused)
{
	auto* ctx = static_cast<WindowContext*>(glfwGetWindowUserPointer(window));
	if (!ctx)
		return;
	InputManager& im = InputManager::get();

	if (!focused) {
		// Clear all input states to prevent stuck keys/buttons
		im._keyStates.fill(KeyState::RELEASED);
		im._mouseButtonStates.fill(KeyState::RELEASED);
		im._mouseDelta = glm::vec2(0.0f);
		im._scroll     = glm::vec2(0.0f);
	}
}
void InputManager::setMouseTrackingEnabled(bool enabled)
{
	if (mouseTrackingEnabled == enabled)
		return; // No change
	mouseTrackingEnabled = enabled;

	if (enabled) {
		// Hide cursor and capture it (disable OS cursor)
		glfwSetInputMode(_context->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
		// Show cursor and release capture (enable OS cursor)
		glfwSetInputMode(_context->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	_mouseDelta = {};
}
