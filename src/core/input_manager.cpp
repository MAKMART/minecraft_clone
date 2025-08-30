#include "core/input_manager.hpp"

InputManager::InputManager(GLFWwindow& window) : _window(window)
{
	glfwSetWindowUserPointer(&_window, this);
	glfwSetKeyCallback(&_window, key_callback);
	glfwSetMouseButtonCallback(&_window, mouse_button_callback);
	glfwSetCursorPosCallback(&_window, cursor_pos_callback);
	glfwSetScrollCallback(&_window, scroll_callback);
	glfwSetWindowFocusCallback(&_window, window_focus_callback);
	//glfwGetCursorPos(&_window, &_lastMouse.x, &_lastMouse.y);
}

void InputManager::update()
{
	_mouseDelta = glm::vec2(0.0f); // reset after each frame
	_scroll = glm::vec2(0.0f);
	_previousKeyStates = _keyStates;
	_previousMouseButtonStates = _mouseButtonStates;
	for (auto& key : _keyStates) {
		if (key == KeyState::PRESSED) key = KeyState::HELD;
	}
	for (auto& btn : _mouseButtonStates) {
		if (btn == KeyState::PRESSED) btn = KeyState::HELD;
	}
}
void InputManager::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* im = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!im || key < 0 || key >= MAX_KEYS) return;

    if (action == GLFW_PRESS)
        im->_keyStates[key] = KeyState::PRESSED;
    else if (action == GLFW_RELEASE)
        im->_keyStates[key] = KeyState::RELEASED;
}

void InputManager::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto* im = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!im || button < 0 || button >= MAX_MOUSE_BUTTONS) return;

    if (action == GLFW_PRESS)
        im->_mouseButtonStates[button] = KeyState::PRESSED;
    else if (action == GLFW_RELEASE)
        im->_mouseButtonStates[button] = KeyState::RELEASED;
}

void InputManager::cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    auto* im = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!im) return;

    glm::vec2 newMouse(static_cast<float>(xpos), static_cast<float>(ypos));
    im->_mouseDelta += newMouse - im->_lastMouse;
    im->_lastMouse = newMouse;

    if (im->invertYAxis) im->_mouseDelta.y *= -1.0f;
    if (im->invertXAxis) im->_mouseDelta.x *= -1.0f;
}

void InputManager::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto* im = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!im) return;

    im->_scroll += glm::vec2(static_cast<float>(xoffset), static_cast<float>(yoffset));
}

void InputManager::window_focus_callback(GLFWwindow* window, int focused) {
    auto* im = static_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (!im) return;

    if (!focused) {
        // Clear all input states to prevent stuck keys/buttons
        im->_keyStates.fill(KeyState::RELEASED);
        im->_mouseButtonStates.fill(KeyState::RELEASED);
        im->_mouseDelta = glm::vec2(0.0f);
        im->_scroll = glm::vec2(0.0f);
    }
}
// --- Keyboard Functions ---
bool InputManager::isPressed(int_fast16_t key) const
{
	return key < MAX_KEYS && _keyStates[key] == KeyState::PRESSED;
}

bool InputManager::isHeld(int_fast16_t key) const
{
	return key < MAX_KEYS && _keyStates[key] == KeyState::HELD;
}

bool InputManager::isReleased(int_fast16_t key) const
{
	return key < MAX_KEYS && _previousKeyStates[key] != KeyState::RELEASED && _keyStates[key] == KeyState::RELEASED;
}

// --- Mouse Button Functions ---
bool InputManager::isMousePressed(int_fast8_t button) const
{
	return button < MAX_MOUSE_BUTTONS && _mouseButtonStates[button] == KeyState::PRESSED;
}

bool InputManager::isMouseHeld(int_fast8_t button) const
{
	return button < MAX_MOUSE_BUTTONS && _mouseButtonStates[button] == KeyState::HELD;
}

bool InputManager::isMouseReleased(int_fast8_t button) const
{
	return button < MAX_MOUSE_BUTTONS && _previousMouseButtonStates[button] != KeyState::RELEASED && _mouseButtonStates[button] == KeyState::RELEASED;
}

// --- Mouse Movement Functions ---
const glm::vec2& InputManager::getMousePosition() const
{
	return _lastMouse;
}

glm::vec2 InputManager::getMouseDelta() const
{
	return glm::vec2(
	    invertXAxis ? -_mouseDelta.x : _mouseDelta.x,
	    invertYAxis ? -_mouseDelta.y : _mouseDelta.y
	    );
}
void InputManager::setMouseTrackingEnabled(bool enabled)
{
	if (mouseTrackingEnabled == enabled)
		return; // No change

	mouseTrackingEnabled = enabled;

	if (enabled) {
		// Hide cursor and capture it (disable OS cursor)
		glfwSetInputMode(&_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	} else {
		// Show cursor and release capture (enable OS cursor)
		glfwSetInputMode(&_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}
