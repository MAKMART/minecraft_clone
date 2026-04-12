export module debug;

import std;
import ecs_components;

export {
  std::string_view player_mode_to_string(PlayerMode mode) noexcept
  {
    switch (mode.mode) {
      case Type::SURVIVAL:  return "SURVIVAL";
      case Type::CREATIVE:  return "CREATIVE";
      case Type::SPECTATOR: return "SPECTATOR";
      default:              return "UNHANDLED";
    }
  }

  std::string_view player_state_to_string(PlayerState state) noexcept
  {
    switch (state.current) {
      case PlayerMovementState::Idle:       return "IDLE";
      case PlayerMovementState::Walking:    return "WALKING";
      case PlayerMovementState::Running:    return "RUNNNIG";
      case PlayerMovementState::Jumping:    return "JUMPING";
      case PlayerMovementState::Crouching:  return "CROUCHING";
      case PlayerMovementState::Flying:     return "FLYING";
      case PlayerMovementState::Swimming:   return "SWIMMING";
      case PlayerMovementState::Falling:    return "FALLING";
      default:                              return "UNHANDLED";
    }
  }
}
