export module engine.platform:input_action_map;
import std;
import engine.core;
import :input_state;
export namespace engine::platform {
  using action_id = u32;
  class input_action_map final {
    public:
      [[nodiscard("You'd want to probably store the action binding, don't ya think?")]] 
        action_id create_action(const std::string& name) noexcept {

          if (auto it = _name_to_id.find(name); it != _name_to_id.end()) {
            return it->second;
          }

          action_id id = static_cast<action_id>(_actions.size());

          _actions.emplace_back();

          _name_to_id[name] = id;

          return id;
        };

      void bind_key(action_id action, button b) noexcept {
        if (action >= _actions.size())
          return;
        if (!std::ranges::contains(_actions[action].keys, b))
          _actions[action].keys.push_back(b);
      };

      void unbind_key(action_id action, button b) noexcept {
        if (action >= _actions.size())
          return;
        auto it = std::find(_actions[action].keys.begin(), _actions[action].keys.end(), b);
        if (it != _actions[action].keys.end())
          _actions[action].keys.erase(it);
      };

      [[nodiscard]] bool is_action_pressed(action_id action, const input_state& input) const noexcept {
        for (auto key : _actions[action].keys) {
          if (input.is_pressed(key))
            return true;
        }
        return false;
      };

      [[nodiscard]] bool is_action_held(action_id action, const input_state& input) const noexcept {
        for (auto key : _actions[action].keys) {
          if (input.is_held(key) || input.is_pressed(key))
            return true;
        }
        return false;
      };

      [[nodiscard]] bool is_action_released(action_id action, const input_state& input) const noexcept {
        for (auto key : _actions[action].keys) {
          if (input.is_released(key))
            return true;
        }
        return false;
      };

    private:
      struct action_binding {
        std::vector<button> keys;
      };

      std::vector<action_binding> _actions;

      std::unordered_map<std::string, action_id> _name_to_id;
  };
}
