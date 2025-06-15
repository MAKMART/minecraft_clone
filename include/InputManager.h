#pragma once
#include <cstdint>
#include <GLFW/glfw3.h>
#include <array>
#include <utility> // for std::pair

class InputManager {
  public:
    enum class KeyState { RELEASED,
                          PRESSED,
                          HELD };

    InputManager(GLFWwindow *window);

    // --- Keyboard Input ---
    bool isPressed(int_fast16_t key) const;
    bool isHeld(int_fast16_t key) const;
    bool isReleased(int_fast16_t key) const;

    // --- Mouse Input ---
    bool isMousePressed(int_fast8_t button) const;
    bool isMouseHeld(int_fast8_t button) const;
    bool isMouseReleased(int_fast8_t button) const;
    double getMouseX(void) const { return _lastMouseX; }
    double getMouseY(void) const { return _lastMouseY; }

    // Get mouse position
    std::pair<double, double> getMousePosition() const;
    std::pair<double, double> getMouseDelta() const;

    // Update function (called once per frame)
    void update(void);

    bool invertYAxis = false;
    void toggleInvertYAxis(void) {
        invertYAxis = !invertYAxis;
    }
    bool isMouseYaxisInverted(void) {
        return invertYAxis;
    }

  private:
    GLFWwindow *_window;

    // Using fixed-size arrays instead of unordered_map for faster lookup
    static constexpr int MAX_KEYS = GLFW_KEY_LAST + 1;
    static constexpr int MAX_MOUSE_BUTTONS = GLFW_MOUSE_BUTTON_LAST + 1;

    std::array<KeyState, MAX_KEYS> _keyStates{};
    std::array<KeyState, MAX_KEYS> _previousKeyStates{};

    std::array<KeyState, MAX_MOUSE_BUTTONS> _mouseButtonStates{};
    std::array<KeyState, MAX_MOUSE_BUTTONS> _previousMouseButtonStates{};

    double _lastMouseX = 0.0, _lastMouseY = 0.0;
    double _mouseDeltaX = 0.0, _mouseDeltaY = 0.0;
};
