module;
#include <gl.h>
#include <imgui.h>
export module debug;

import std;
import ecs_components;
import app_state;
import glm;
import core;
import gl_state;
import timer;
import chunk_manager;
import input_manager;

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
  X(GL_STENCIL_INDEX8) \
  X(GL_DEBUG_SEVERITY_HIGH) \
  X(GL_DEBUG_SEVERITY_MEDIUM) \
  X(GL_DEBUG_SEVERITY_LOW) \
  X(GL_DEBUG_SEVERITY_NOTIFICATION) \
  X(GL_DEBUG_SOURCE_API) \
  X(GL_DEBUG_SOURCE_WINDOW_SYSTEM) \
  X(GL_DEBUG_SOURCE_SHADER_COMPILER) \
  X(GL_DEBUG_SOURCE_THIRD_PARTY) \
  X(GL_DEBUG_SOURCE_APPLICATION) \
  X(GL_DEBUG_SOURCE_OTHER) \
  X(GL_DEBUG_TYPE_ERROR) \
  X(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR) \
  X(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR) \
  X(GL_DEBUG_TYPE_PERFORMANCE) \
  X(GL_DEBUG_TYPE_MARKER) \
  X(GL_DEBUG_TYPE_PUSH_GROUP) \
  X(GL_DEBUG_TYPE_POP_GROUP) \
  X(GL_DEBUG_TYPE_PORTABILITY) \
  X(GL_ARRAY_BUFFER) \
  X(GL_ELEMENT_ARRAY_BUFFER) \
  X(GL_SHADER_STORAGE_BUFFER) \
  X(GL_UNIFORM_BUFFER) \

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

  constexpr std::string_view gl_enum_to_string(GLenum e) {
    switch (e) {
#define X(x) case x: return #x;
      GL_ENUM_LIST
#undef X
      default: return "UNKNOWN_GL_ENUM";
    }
  }

#if defined(DEBUG)
  void DrawBool(const char* label, bool value)
  {
    ImGui::Text("%s: ", label);
    ImGui::SameLine();
    ImGui::TextColored(value ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), value ? "TRUE" : "FALSE");
  }
  void setup_imgui(app_state& state, const ChunkManager& manager, CameraController* ctrl, const PlayerState* player_state) {
    ImGui::NewFrame();
    auto pos = state.g_state.ecs.get_component<Transform>(state.g_state.player.self)->pos;
    auto vel = state.g_state.ecs.get_component<Velocity>(state.g_state.player.self)->value;
    glm::ivec3 chunkCoords = world_to_chunk(pos);
    glm::ivec3 localCoords = world_to_local(pos);
    ImGui::Begin("DEBUG", NULL,
        ImGuiWindowFlags_::ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoNavInputs |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_::ImGuiWindowFlags_NoCollapse);
    ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable("Rendering Infos:", false, ImGuiSelectableFlags_Disabled);
    ImGui::PopStyleColor(1);
    ImGui::PopFont();
    ImGui::Indent();
    ImGui::Text("FPS: %f", getFPS(state.frame_ctx.delta_time));
    ImGui::Text("Draw Calls: %d", g_drawCallCount);
    RenderTimings();
    ImGui::Unindent();
    ImGui::Spacing();
    ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable("Player:", false, ImGuiSelectableFlags_Disabled);
    ImGui::PopStyleColor(1);
    ImGui::PopFont();
    ImGui::Indent();
    ImGui::Text("Player position: %f, %f, %f", pos.x, pos.y, pos.z);
    ImGui::Text("Player is in chunk: %i, %i, %i", chunkCoords.x, chunkCoords.y, chunkCoords.z);
    ImGui::Text("Player local position: %d, %d, %d", localCoords.x, localCoords.y, localCoords.z);
    ImGui::Text("Player velocity: %f, %f, %f", vel.x, vel.y, vel.z);
    ImGui::Text("Player STATE: %s", player_mode_to_string(*state.g_state.ecs.get_component<PlayerMode>(state.g_state.player.self)).data());
    ImGui::Text("Player STATE: %s", player_state_to_string(*state.g_state.ecs.get_component<PlayerState>(state.g_state.player.self)).data());
    // glm::vec3 _min = player.getAABB().min;
    // glm::vec3 _max = player.getAABB().max;
    // ImGui::Text("Player AABB { %f, %f, %f }, { %f, %f, %f }", _min.x, _min.y, _min.z, _max.x, _max.y, _max.z);
    ImGui::Text("Selected block: ");
    ImGui::SameLine();
    ImGui::SetWindowFontScale(1.2f);
    ImGui::Text("%s", Block::toString(static_cast<Block::blocks>(state.g_state.player.selectedBlock)));
    ImGui::SetWindowFontScale(1.0f);
    ImGui::Unindent();
    ImGui::Spacing();
    ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::Selectable("Player flags:", false, ImGuiSelectableFlags_Disabled);
    ImGui::PopStyleColor(1);
    ImGui::PopFont();
    ImGui::Indent();
    DrawBool("is OnGround", state.g_state.ecs.get_component<Collider>(state.g_state.player.self)->is_on_ground);
    DrawBool("is Damageable", state.g_state.player.isDamageable);
    DrawBool("is Running", player_state->current == PlayerMovementState::Running);
    DrawBool("is Flying", player_state->current == PlayerMovementState::Flying);
    DrawBool("is Swimming", player_state->current == PlayerMovementState::Swimming);
    DrawBool("is Walking", player_state->current == PlayerMovementState::Walking);
    DrawBool("is Crouched", player_state->current == PlayerMovementState::Crouching);
    DrawBool("is third-person", ctrl->third_person);
    DrawBool("Player can place blocks", state.g_state.player.canPlaceBlocks);
    DrawBool("Player can break blocks", state.g_state.player.canBreakBlocks);
    DrawBool("renderSkin", state.g_state.player.renderSkin);
    ImGui::Unindent();
    ImGui::Spacing();
    ImGui::PushFont(ImGui::GetFont()); // Or use a bold/large font if you have one
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
    ImGui::Selectable("Camera:", false, ImGuiSelectableFlags_Disabled);
    ImGui::PopStyleColor(1);
    ImGui::PopFont();
    ImGui::Indent();
    // DrawBool("is Camera interpolating", player.getCameraController().isInterpolationEnabled());
    glm::vec3& camPos = state.g_state.ecs.get_component<Transform>(state.frame_ctx.active_camera)->pos;
    ImGui::Text("Camera position: %f, %f, %f", camPos.x, camPos.y, camPos.z);
    ImGui::Unindent();
    if (state.g_state.render_ui && !InputManager::get().isMouseTrackingEnabled()) {
      if (ImGui::CollapsingHeader("Settings")) {
        if (ImGui::TreeNode("Player")) {
          auto* cfg = state.g_state.ecs.get_component<MovementConfig>(state.g_state.player.self);
          ImGui::SliderFloat("Player walking speed ", &cfg->walk_speed, 0.0f, 100.0f);
          ImGui::SliderFloat("Player flying speed", &cfg->fly_speed, 0.0f, 100.0f);
          ImGui::SliderFloat("Max Interaction Distance", &state.g_state.player.max_interaction_distance, 0.0f, 1000.0f);
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Camera")) {
          // ImGui::SliderFloat("Mouse Sensitivity", &ctrl->sensitivity, 0.01f, 2.0f);
          ImGui::SliderFloat("FOV", &state.frame_ctx.cam->fov, 0.001f, 179.899f);
          ImGui::SliderFloat("Near Plane", &state.frame_ctx.cam->near_plane, 0.001f, 10.0f);
          ImGui::SliderFloat("Far Plane", &state.frame_ctx.cam->far_plane, 10.0f, 1000000.0f);
          ImGui::SliderFloat("Third Person Offset", &ctrl->orbit_distance, 1.0f, 200.0f);
          ImGui::TreePop();
        }
        if (ImGui::TreeNode("Miscellaneous")) {
          ImGui::SliderInt("Render distance", (int*)&state.g_state.player.render_distance, 0, 30);
          ImGui::SliderFloat("GRAVITY", &GRAVITY, -30.0f, 20.0f);
          glm::vec4 backgroundColor{GLState::get_clear_color()};
          ImGui::SliderFloat3("BACKGROUND COLOR", &backgroundColor.r, 0.0f, 1.0f);
          GLState::set_clear_color(backgroundColor);
          static float _____width = GLState::get_line_width();
          ImGui::SliderFloat("LINE_WIDTH", &_____width, 0.001f, 9.0f);
          GLState::set_line_width(_____width);
          ImGui::Checkbox("render_terrain: ", &state.g_state.render_terrain);
          ImGui::Checkbox("render_player: ", &state.g_state.player.renderSkin);
          // static bool debug = false;
          // ImGui::Checkbox("debug", &debug); // Updates the value
          // manager.getShader().setBool("debug", debug);
          ImGui::TreePop();
        }
      }
    }
    ImGui::End();
  }
#endif

}
