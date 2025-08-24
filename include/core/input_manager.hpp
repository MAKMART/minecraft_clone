#pragma once
#include <cstdint>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <utility> // for std::pair

class InputManager {
  public:
    enum class KeyState { RELEASED, PRESSED, HELD };

    InputManager(GLFWwindow *window);

    // --- Keyboard Input ---
    bool isPressed(int_fast16_t key) const;
    bool isHeld(int_fast16_t key) const;
    bool isReleased(int_fast16_t key) const;

    // --- Mouse Input ---
    bool isMousePressed(int_fast8_t button) const;
    bool isMouseHeld(int_fast8_t button) const;
    bool isMouseReleased(int_fast8_t button) const;
    double getMouseX() const { return _lastMouseX; }
    double getMouseY() const { return _lastMouseY; }

    std::pair<double, double> getMousePosition() const;
    std::pair<double, double> getMouseDelta() const;

    void update();

    void setInvertYAxis(bool enabled) {
        if (invertYAxis != enabled) {
            invertYAxis = enabled;
        }
    }
    bool isMouseYAxisInverted() {
        return invertYAxis;
    }
    void setInvertXAxis(bool enabled) {
        if (invertXAxis != enabled) {
            invertXAxis = enabled;
        }
    }
    bool isMouseXAxisInverted() {
        return invertXAxis;
    }
    bool isMouseTrackingEnabled() const { return mouseTrackingEnabled; }
    void setMouseTrackingEnabled(bool enabled);


  private:
    GLFWwindow *_window;
    bool invertYAxis = false;
    bool invertXAxis = false;

    static constexpr int MAX_KEYS = GLFW_KEY_LAST + 1;
    static constexpr int MAX_MOUSE_BUTTONS = GLFW_MOUSE_BUTTON_LAST + 1;

    std::array<KeyState, MAX_KEYS> _keyStates{};
    std::array<KeyState, MAX_KEYS> _previousKeyStates{};

    std::array<KeyState, MAX_MOUSE_BUTTONS> _mouseButtonStates{};
    std::array<KeyState, MAX_MOUSE_BUTTONS> _previousMouseButtonStates{};

    bool mouseTrackingEnabled = true;

    double _lastMouseX = 0.0, _lastMouseY = 0.0;
    double _mouseDeltaX = 0.0, _mouseDeltaY = 0.0;
};
