module;
#include <GLFW/glfw3.h>
#include <cassert>
export module engine.platform:input_state;

import std;
import engine.math;
import engine.core;

using engine::math::dvec2;
using engine::core::button;
using engine::core::cursor_mode;
using namespace engine::core::events;
export namespace engine::platform {
  constexpr button glfw_to_engine_button(i32 glfw_key) {
    switch (glfw_key)
    {
      case GLFW_KEY_W:
        return button::w;
      case GLFW_KEY_A:
        return button::a;
      case GLFW_KEY_SPACE:
        return button::space;
      case GLFW_KEY_ESCAPE:
        return button::escape;
      case GLFW_MOUSE_BUTTON_RIGHT:
        return button::mouse_right;
      case GLFW_MOUSE_BUTTON_MIDDLE:
        return button::mouse_middle;
      case GLFW_MOUSE_BUTTON_LEFT:
        return button::mouse_left;
      default:
        return button::unknown;
    }
  }
  class input_state final {
    public:
      input_state(event_dispatcher& _dispatcher) : dispatcher(_dispatcher) {
        dispatcher.subscribe<key_event>([&](const key_event& key_event) {
            on_key_callback(key_event);
            });
        dispatcher.subscribe<window_focus>([&](const window_focus& window_focus_event) {
            on_window_focus_callback(window_focus_event.focused);
            });
      }
      ~input_state() = default;

      // Delete copy/move constructors
      input_state(const input_state&)            = delete;
      input_state(input_state&&)                 = delete;
      input_state& operator=(const input_state&) = delete;
      input_state& operator=(input_state&&)      = delete;

      // Remember to call this at the end of whatever input pooling that you do trough isPressed/isHeld etc
      void update() noexcept {
        _mouseDelta = dvec2{0.0}; // reset after each frame
        _scroll = dvec2{0.0};
        for (auto& btn : button_states) {
          if (btn == button_state::PRESSED)  btn = button_state::DOWN;
          if (btn == button_state::RELEASED) btn = button_state::UP;
        }
      };

      // --- Queries ---
      [[nodiscard]] bool is_pressed(button b) const noexcept {
        auto idx = static_cast<std::size_t>(b);
        assert(idx < button_count && "Invalid button");
        return button_states[idx] == button_state::PRESSED;
      };
      [[nodiscard]] bool is_held(button b) const noexcept {
        auto idx = static_cast<std::size_t>(b);
        assert(idx < button_count && "Invalid button");
        return button_states[idx] == button_state::DOWN || button_states[idx] == button_state::PRESSED;
      };
      [[nodiscard]] bool is_released(button b) const noexcept {
        auto idx = static_cast<std::size_t>(b);
        assert(idx < button_count && "Invalid button");
        return button_states[idx] == button_state::RELEASED;
      };

      inline const dvec2& get_mouse_pos() const noexcept { return _lastMouse; }
      inline const dvec2& get_scroll() const noexcept { return _scroll; }

      inline dvec2 get_mouse_delta() const noexcept {
        return dvec2{
            invert_x ? -_mouseDelta.x : _mouseDelta.x,
            invert_y ? -_mouseDelta.y : _mouseDelta.y};
      }

      bool is_y_axis_inverted() const noexcept { return invert_y; }
      bool is_x_axis_inverted() const noexcept { return invert_x; }
      // bool is_mouse_tracking() const noexcept { return mouse_tracking; }
      bool is_mouse_tracking() const noexcept { return current_cursor_mode == cursor_mode::locked; }

      void invert_mouse_x(bool enabled) noexcept { if (invert_x != enabled) invert_x = enabled; }
      void invert_mouse_y(bool enabled) noexcept { if (invert_y != enabled) invert_y = enabled; }
      void set_cursor_mode(cursor_mode mode) {
        if (current_cursor_mode == mode)
          return;
        auto previous = current_cursor_mode;
        current_cursor_mode = mode;
        dispatcher.dispatch(
            cursor_mode_changed{
            .previous = previous,
            .current = mode
            });
        _mouseDelta = dvec2{0.0};
      };


      void on_scroll_callback(f64 xoffset, f64 yoffset) noexcept {
        _scroll += dvec2{xoffset, yoffset};
      };

      void on_cursor_pos_callback(f64 xpos, f64 ypos) noexcept {
        dvec2 newMouse{xpos, ypos};
        // _mouseDelta = newMouse - _lastMouse;
        // This is weird!!!
        _mouseDelta = engine::math::operator-(newMouse, _lastMouse);
        _lastMouse = newMouse;
      };
      void on_mouse_button_callback(i32 mouse_button, i32 action, i32 mods) noexcept {
        button b = glfw_to_engine_button(mouse_button);
        auto idx = static_cast<std::size_t>(b);
        if (action == GLFW_PRESS)
          button_states[idx] = button_state::PRESSED;
        else if (action == GLFW_RELEASE)
          button_states[idx] = button_state::RELEASED;
      };
      void on_key_callback(const key_event& key_event) noexcept {
        button b = glfw_to_engine_button(key_event.key);
        // core::logger::info("Pressed button: {}", b);

        auto idx = static_cast<std::size_t>(b);

        if (key_event.action == GLFW_PRESS) {
          if (button_states[idx] == button_state::UP || button_states[idx] == button_state::RELEASED) {
            button_states[idx] = button_state::PRESSED;
            // core::logger::system_info("INPUT", "Pressed key: {}", key_event.key);
          }
        }
        else if (key_event.action == GLFW_RELEASE)
          button_states[idx] = button_state::RELEASED;
        else if (key_event.action == GLFW_REPEAT)
          return;
      };
      void on_window_focus_callback(bool focused) noexcept {
        if (!focused) {
          // Clear all input states to prevent stuck keys/buttons
          button_states.fill(button_state::UP);
          _mouseDelta = dvec2{0.0};
          _scroll     = dvec2{0.0};
        }
      };
    private:
      event_dispatcher& dispatcher;
      bool invert_y{false};
      bool invert_x{false};
      cursor_mode current_cursor_mode{cursor_mode::visible};

      enum class button_state : u8 {
        UP,
        PRESSED,
        DOWN,
        RELEASED
      };

      static constexpr std::size_t button_count{static_cast<std::size_t>(button::count)};
      std::array<button_state, button_count> button_states{};

      dvec2 _lastMouse{0.0f};
      dvec2 _mouseDelta{0.0f};
      dvec2 _scroll{0.0f};
  };
}
