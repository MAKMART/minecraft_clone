#include "core/input_manager.hpp"

InputManager::InputManager(GLFWwindow *window) : _window(window) {
    glfwGetCursorPos(_window, &_lastMouseX, &_lastMouseY);
}

void InputManager::update() {
    // Store previous states before updating
    _previousKeyStates = _keyStates;
    _previousMouseButtonStates = _mouseButtonStates;

    // --- Update Keyboard States (ONLY for active keys) ---
    for (int key = GLFW_KEY_SPACE; key < MAX_KEYS; ++key) {
        int glfwState = glfwGetKey(_window, key);
        if (glfwState == GLFW_PRESS) {
            _keyStates[key] = (_previousKeyStates[key] == KeyState::RELEASED) ? KeyState::PRESSED : KeyState::HELD;
        } else {
            _keyStates[key] = KeyState::RELEASED;
        }
    }

    // --- Update Mouse Button States (ONLY for active buttons) ---
    for (int button = GLFW_MOUSE_BUTTON_1; button < MAX_MOUSE_BUTTONS; ++button) {
        int glfwState = glfwGetMouseButton(_window, button);
        if (glfwState == GLFW_PRESS) {
            _mouseButtonStates[button] = (_previousMouseButtonStates[button] == KeyState::RELEASED) ? KeyState::PRESSED : KeyState::HELD;
        } else {
            _mouseButtonStates[button] = KeyState::RELEASED;
        }
    }

    // --- Update Mouse Movement (store only once per frame) ---
    double mouseX, mouseY;
    glfwGetCursorPos(_window, &mouseX, &mouseY);

    _mouseDeltaX = mouseX - _lastMouseX;
    _mouseDeltaY = mouseY - _lastMouseY;

    _lastMouseX = mouseX;
    _lastMouseY = mouseY;
}

// --- Keyboard Functions ---
bool InputManager::isPressed(int_fast16_t key) const {
    return key < MAX_KEYS && _keyStates[key] == KeyState::PRESSED;
}

bool InputManager::isHeld(int_fast16_t key) const {
    return key < MAX_KEYS && _keyStates[key] == KeyState::HELD;
}

bool InputManager::isReleased(int_fast16_t key) const {
    return key < MAX_KEYS && _previousKeyStates[key] != KeyState::RELEASED && _keyStates[key] == KeyState::RELEASED;
}

// --- Mouse Button Functions ---
bool InputManager::isMousePressed(int_fast8_t button) const {
    return button < MAX_MOUSE_BUTTONS && _mouseButtonStates[button] == KeyState::PRESSED;
}

bool InputManager::isMouseHeld(int_fast8_t button) const {
    return button < MAX_MOUSE_BUTTONS && _mouseButtonStates[button] == KeyState::HELD;
}

bool InputManager::isMouseReleased(int_fast8_t button) const {
    return button < MAX_MOUSE_BUTTONS && _previousMouseButtonStates[button] != KeyState::RELEASED && _mouseButtonStates[button] == KeyState::RELEASED;
}

// --- Mouse Movement Functions ---
std::pair<double, double> InputManager::getMousePosition() const {
    return {_lastMouseX, _lastMouseY};
}

std::pair<double, double> InputManager::getMouseDelta() const {
    return {
        invertXAxis ? -_mouseDeltaX : _mouseDeltaX,
        invertYAxis ? -_mouseDeltaY : _mouseDeltaY
    };
}
void InputManager::setMouseTrackingEnabled(bool enabled) {
    if (mouseTrackingEnabled == enabled)
        return; // No change

    mouseTrackingEnabled = enabled;

    if (enabled) {
        // Hide cursor and capture it (disable OS cursor)
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        // Show cursor and release capture (enable OS cursor)
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}
