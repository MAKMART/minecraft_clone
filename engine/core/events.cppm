export module engine.core:events;

import std;
import :types;

export namespace engine::core::events {

  struct key_event {
    i32 key;
    i32 scancode;
    i32 action;
    i32 mods;
  };

  struct mouse_button {
    i32 button;
    i32 action;
    i32 mods;
  };

  struct window_resize {
    u32 width;
    u32 height;
  };

  struct framebuffer_resize {
    u32 width;
    u32 height;
  };
  struct window_close {};

  struct window_focus {
    bool focused;
  };

  struct cursor_mode_changed {
    cursor_mode previous;
    cursor_mode current;
  };

  using event = std::variant<
    key_event,
    mouse_button,
    window_resize,
    framebuffer_resize,
    window_close,
    window_focus,
    cursor_mode_changed
  >;
  class event_dispatcher final {
    public:
      template <typename EventT>
        using callback = std::function<void(const EventT&)>;

      // Subscribe to a specific event type
      template <typename EventT>
        void subscribe(callback<EventT> cb) {
          get_list<EventT>().push_back(std::move(cb));
        }

      // Dispatch ANY variant event
      void dispatch(const event& e) const {
        std::visit([this](auto&& ev) {
            using T = std::decay_t<decltype(ev)>;
            dispatch_single(ev, get_list<T>());
            }, e);
      }

    private:
      // Actual dispatch for a concrete type
      template <typename EventT>
        void dispatch_single(const EventT& ev, const std::vector<callback<EventT>>& list) const {
          for (auto& cb : list)
            cb(ev);
        }

      // One list per event type
      template <typename EventT>
        static std::vector<callback<EventT>>& get_list() {
          static std::vector<callback<EventT>> list;
          return list;
        }
  };

  class event_queue final {
    public:
      void push(event e) {
        queue.push_back(std::move(e));
      }

      void flush(event_dispatcher& dispatcher) {
        for (const auto& e : queue)
          dispatcher.dispatch(e);
        queue.clear();
      }

    private:
      std::vector<event> queue;
  };
}
