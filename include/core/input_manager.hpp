#pragma once
#include <cstdint>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>
#include "core/window_context.hpp"
#include <glm/glm.hpp>

class InputManager
{
      public:

	// Singleton access
	static InputManager& get()
	{
		static InputManager instance;
		return instance;
	}

	// Delete copy/move constructors
	InputManager(const InputManager&)            = delete;
	InputManager(InputManager&&)                 = delete;
	InputManager& operator=(const InputManager&) = delete;
	InputManager& operator=(InputManager&&)      = delete;

	// Remember to call this at the end of whatever input pooling that you do trough isPressed/isHeld etc
	void update();

	// --- Keyboard Input ---
	[[nodiscard]] inline bool isPressed(int_fast16_t key) const noexcept
	{
		assert(key >= 0 && key < MAX_KEYS && "Invalid key code");
		return _keyStates[key] == KeyState::PRESSED;
	}

	[[nodiscard]] inline bool isHeld(int_fast16_t key) const noexcept
	{
		assert(key >= 0 && key < MAX_KEYS && "Invalid key code");
		return _keyStates[key] == KeyState::DOWN;
	}

	[[nodiscard]] inline bool isReleased(int_fast16_t key) const noexcept
	{
		assert(key >= 0 && key < MAX_KEYS && "Invalid key code");
		return _keyStates[key] == KeyState::RELEASED;
	}

	// --- Mouse Input ---
	[[nodiscard]] inline bool isMousePressed(int_fast8_t button) const noexcept
	{
		assert(button >= 0 && button < MAX_MOUSE_BUTTONS && "Invalid button code");
		return _mouseButtonStates[button] == KeyState::PRESSED;
	}

	[[nodiscard]] inline bool isMouseHeld(int_fast8_t button) const noexcept
	{
		assert(button >= 0 && button < MAX_MOUSE_BUTTONS && "Invalid button code");
		return _mouseButtonStates[button] == KeyState::DOWN;
	}

	[[nodiscard]] inline bool isMouseReleased(int_fast8_t button) const noexcept
	{
		assert(button >= 0 && button < MAX_MOUSE_BUTTONS && "Invalid button code");
		return _mouseButtonStates[button] == KeyState::RELEASED;
	}

	inline const glm::vec2& getMousePos() const noexcept { return _lastMouse; }
	inline const glm::vec2& getScroll() const noexcept { return _scroll; }

	inline glm::vec2 getMouseDelta() const noexcept {
		return glm::vec2(
		    invertXAxis ? -_mouseDelta.x : _mouseDelta.x,
		    invertYAxis ? -_mouseDelta.y : _mouseDelta.y);
	}


	bool isMouseYAxisInverted() const noexcept { return invertYAxis; }
	bool isMouseXAxisInverted() const noexcept { return invertXAxis; }
	bool isMouseTrackingEnabled() const noexcept { return mouseTrackingEnabled; }

	void setInvertXAxis(bool enabled) noexcept
	{
		if (invertXAxis != enabled) {
			invertXAxis = enabled;
		}
	}
	void setInvertYAxis(bool enabled) noexcept
	{
		if (invertYAxis != enabled) {
			invertYAxis = enabled;
		}
	}
	void setMouseTrackingEnabled(bool enabled);

	static constexpr int MAX_KEYS          = GLFW_KEY_LAST + 1;
	static constexpr int MAX_MOUSE_BUTTONS = GLFW_MOUSE_BUTTON_LAST + 1;

	void setContext(WindowContext* context)
	{
		_context = context;
		if (_context && _context->window) {
			registerCallbacks();
		}
		if (mouseTrackingEnabled) {
			// Hide cursor and capture it (disable OS cursor)
			glfwSetInputMode(_context->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		} else {
			// Show cursor and release capture (enable OS cursor)
			glfwSetInputMode(_context->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

	}

      private:
	InputManager()  = default;
	~InputManager() = default;
	WindowContext* _context;
	bool           invertYAxis = false;
	bool           invertXAxis = false;

	enum class KeyState { UP, PRESSED, DOWN, RELEASED };
	std::array<KeyState, MAX_KEYS> _keyStates{};
	std::array<KeyState, MAX_MOUSE_BUTTONS> _mouseButtonStates{};

	bool mouseTrackingEnabled = true;

	void registerCallbacks()
	{
		GLFWwindow* win = _context->window;
		glfwSetKeyCallback(win, key_callback);
		glfwSetMouseButtonCallback(win, mouse_button_callback);
		glfwSetCursorPosCallback(win, cursor_pos_callback);
		glfwSetScrollCallback(win, scroll_callback);
		glfwSetWindowFocusCallback(win, window_focus_callback);
	}
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void window_focus_callback(GLFWwindow* window, int focused);

	glm::vec2 _lastMouse{0.0f};
	glm::vec2 _mouseDelta{0.0f};
	glm::vec2 _scroll{0.0f};
};
