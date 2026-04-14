module;
#include <gl.h>
export module debug;

import std;
import ecs_components;

#define GL_ENUM_LIST \
  X(GL_R8) \
  X(GL_R16) \
  X(GL_RG8) \
  X(GL_RG16) \
  X(GL_RGB8) \
  X(GL_RGBA8) \
  X(GL_RGB10_A2) \
  X(GL_RGBA4) \
  X(GL_RGB5_A1) \
  X(GL_R16F) \
  X(GL_RG16F) \
  X(GL_RGB16F) \
  X(GL_RGBA16F) \
  X(GL_R32F) \
  X(GL_RG32F) \
  X(GL_RGB32F) \
  X(GL_RGBA32F) \
  X(GL_DEPTH_COMPONENT16) \
  X(GL_DEPTH_COMPONENT24) \
  X(GL_DEPTH_COMPONENT32) \
  X(GL_DEPTH_COMPONENT32F) \
  X(GL_DEPTH24_STENCIL8) \
  X(GL_DEPTH32F_STENCIL8) \
  X(GL_STENCIL_INDEX8)
export {
  std::string_view player_mode_to_string(PlayerMode mode) noexcept
  {
    switch (mode.mode) {
      case ModeType::SURVIVAL:  return "SURVIVAL";
      case ModeType::CREATIVE:  return "CREATIVE";
      case ModeType::SPECTATOR: return "SPECTATOR";
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

  std::string_view gl_enum_to_string(GLenum e) {
    switch (e) {
#define X(x) case x: return #x;
      GL_ENUM_LIST
#undef X
      default: return "UNKNOWN_GL_ENUM";
    }
  }
}
