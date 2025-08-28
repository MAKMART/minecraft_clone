#pragma once
#include <cstdint>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <glm/glm.hpp>

class InputManager
{
      public:
	enum class KeyState { RELEASED, PRESSED, HELD };

	InputManager(GLFWwindow* window);

	// --- Keyboard Input ---
	bool isPressed(int_fast16_t key) const;
	bool isHeld(int_fast16_t key) const;
	bool isReleased(int_fast16_t key) const;

	// --- Mouse Input ---
	bool   isMousePressed(int_fast8_t button) const;
	bool   isMouseHeld(int_fast8_t button) const;
	bool   isMouseReleased(int_fast8_t button) const;

	float getMouseX() const
	{
		return _lastMouse.x;
	}
	float getMouseY() const
	{
		return _lastMouse.y;
	}

	const glm::vec2& getMousePosition() const;
	glm::vec2 getMouseDelta() const;

	void update();

	glm::vec2 getScroll() const
	{
		return _scroll;
	}

	float getScrollX() const
	{
		return _scroll.x;
	}
	float getScrollY() const
	{
		return _scroll.y;
	}

	void setInvertYAxis(bool enabled)
	{
		if (invertYAxis != enabled) {
			invertYAxis = enabled;
		}
	}
	bool isMouseYAxisInverted()
	{
		return invertYAxis;
	}
	void setInvertXAxis(bool enabled)
	{
		if (invertXAxis != enabled) {
			invertXAxis = enabled;
		}
	}
	bool isMouseXAxisInverted()
	{
		return invertXAxis;
	}
	bool isMouseTrackingEnabled() const
	{
		return mouseTrackingEnabled;
	}
	void setMouseTrackingEnabled(bool enabled);

	static constexpr int MAX_KEYS          = GLFW_KEY_LAST + 1;
	static constexpr int MAX_MOUSE_BUTTONS = GLFW_MOUSE_BUTTON_LAST + 1;

      private:
	GLFWwindow* _window;
	bool        invertYAxis = false;
	bool        invertXAxis = false;


	std::array<KeyState, MAX_KEYS> _keyStates{};
	std::array<KeyState, MAX_KEYS> _previousKeyStates{};

	std::array<KeyState, MAX_MOUSE_BUTTONS> _mouseButtonStates{};
	std::array<KeyState, MAX_MOUSE_BUTTONS> _previousMouseButtonStates{};

	bool mouseTrackingEnabled = true;


	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void window_focus_callback(GLFWwindow* window, int focused);


	glm::vec2 _lastMouse{0.0f};
	glm::vec2 _mouseDelta{0.0f};
	glm::vec2 _scroll{0.0f};
};
