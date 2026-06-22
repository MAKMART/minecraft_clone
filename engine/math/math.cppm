export module engine.math;

export import :aabb;
export import :types;
export namespace engine::math {
  namespace space {
    inline constexpr vec3 WORLD_FORWARD{0, 0, 1};
    inline constexpr vec3 WORLD_UP{0, 1, 0};
    inline constexpr vec3 WORLD_RIGHT{1, 0, 0};
  }

  namespace camera {
    // constexpr float PITCH_LIMIT = 89.0f;
    // inline void apply_mouse_look(glm::vec2 delta, float sensitivity, float& yaw, float& pitch)
    // {
    //   yaw   -= delta.x * sensitivity;
    //   pitch -= delta.y * sensitivity;
    //
    //   pitch = glm::clamp( pitch, -PITCH_LIMIT, PITCH_LIMIT);
    // }
    //
    // inline glm::quat to_orientation(const float& yaw, const float& pitch)
    // {
    //   glm::quat q_yaw   = glm::angleAxis(glm::radians(yaw), space::WORLD_UP);
    //
    //   glm::quat q_pitch = glm::angleAxis(glm::radians(pitch), space::WORLD_RIGHT);
    //
    //   return glm::normalize(q_yaw * q_pitch);
    // }
  }
}
