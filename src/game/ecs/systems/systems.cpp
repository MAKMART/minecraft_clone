module;
#include <gl.h>
#include <cassert>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
module ecs_systems;

import std;
import core;
import gl_state;
import glm;
import ecs_components;
import chunk;
import input_manager;
import aabb;
import logger;

void camera_pose_system(ECS& ecs, Entity player, const frame_context& ctx, float dt)
{
#if defined(TRACY_ENABLE)
  ZoneScoped;
#endif
  CameraLook* angles = ecs.get_component<CameraLook>(ctx.active_camera);
  Transform* trans = ecs.get_component<Transform>(ctx.active_camera);
  InputComponent* camera_ic = ecs.get_component<InputComponent>(ctx.active_camera);
  if (!angles || !trans || !camera_ic) return;

  if (ecs.has_component<DebugCamera>(ctx.active_camera)) {
    float speed = 10.0f; // units per second
    if (camera_ic->sprint)
      speed = 100.0f;


    // Movement
    if(camera_ic->forward)  trans->pos -= trans->forward() * speed * dt;
    if(camera_ic->backward) trans->pos += trans->forward() * speed * dt;
    if(camera_ic->left)     trans->pos -= trans->right() * speed * dt;
    if(camera_ic->right)    trans->pos += trans->right() * speed * dt;
    if(camera_ic->jump)     trans->pos += trans->up() * speed * dt;
    if(camera_ic->crouch)   trans->pos -= trans->up() * speed * dt;

    // Mouse look
    if (InputManager::get().isMouseTrackingEnabled()) {
      glm::vec2 delta = camera_ic->mouse_delta;
      camera_model::apply_mouse_look(delta, angles->sensitivity, angles->yaw, angles->pitch);
      trans->rot = camera_model::to_orientation(angles->yaw, angles->pitch);
    }

  }

  CameraController* ctrl = ecs.get_component<CameraController>(ctx.active_camera);
  if (!ctrl || !ctrl->target.is_valid()) return;
  Transform* target_trans = ecs.get_component<Transform>(ctrl->target);
  InputComponent* input = ecs.get_component<InputComponent>(player);

  if (!trans || !input || !target_trans)
    log::system_error("camera_controller_system", "the 'active camera' with id : {} doesn't have a Transform || InputComponent component", ctx.active_camera.id);

  if (InputManager::get().isMouseTrackingEnabled()) {
    camera_model::apply_mouse_look(input->mouse_delta, angles->sensitivity, angles->yaw, angles->pitch);
  }

  glm::vec3 targetPos = target_trans->pos + target_trans->rot * ctrl->offset;

  glm::quat rot = camera_model::to_orientation(angles->yaw, angles->pitch);
  glm::vec3 offset{};
  if (ctrl->third_person) {
    offset = rot * (coord::WORLD_FORWARD * ctrl->orbit_distance);
  }
  // Effective target position = entity position + offset
  trans->pos = targetPos + offset;
  trans->rot = rot;
}

void chunk_renderer_system(ECS& ecs, ChunkManager& cm, Entity cam, const FrustumVolume& wanted_fv, const FramebufferManager& fb)
{
#if defined(TRACY_ENABLE)
  ZoneScoped;
#endif
  const auto& cur_fb = fb.get(cam);
  cur_fb.bind_draw();
  GLState::set_viewport(0, 0, cur_fb.width(), cur_fb.height());
  // GLState::is_depth_test() ensures we only clear DEPTH if it's active
  GLbitfield clear_mask = GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
  if (GLState::is_depth_test()) {
    clear_mask |= GL_DEPTH_BUFFER_BIT;
  }
  glClear(clear_mask);

  GLState::set_face_culling(true);
  GLState::set_blending(false);

  const Transform& ts = *ecs.get_component<Transform>(cam);
  cm.render_opaque(ts, wanted_fv);

  //GLState::set_face_culling(false); // Water usually needs both sides or special culling
  //GLState::set_blending(true);
  //GLState::set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  //cm.render_transparent(ts, wanted_fv);
}


void input_system(ECS& ecs)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
  auto& input = InputManager::get();
	ecs.for_each_component<InputComponent>([&](const Entity e, InputComponent& ic) {
		// Reset state
		ic.forward  = input.isHeld(FORWARD_KEY);
		ic.backward = input.isHeld(BACKWARD_KEY);
		ic.left     = input.isHeld(LEFT_KEY);
		ic.right    = input.isHeld(RIGHT_KEY);
		ic.jump     = input.isHeld(JUMP_KEY);
		ic.sprint   = input.isHeld(SPRINT_KEY);
		ic.crouch   = input.isHeld(CROUCH_KEY);

		ic.mouse_delta  = input.getMouseDelta();
		ic.scroll_delta = input.getScroll();

		ic.is_primary_pressed   = input.isMousePressed(ATTACK_BUTTON);
		ic.is_secondary_pressed = input.isMousePressed(DEFENSE_BUTTON);
		ic.is_ternary_pressed   = input.isMousePressed(TERNARY_BUTTON);
	});
}

void movement_intent_system(ECS& ecs, const Entity camera)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
  Transform* trans = ecs.get_component<Transform>(camera);
  assert(trans && "camera transform was nullptr in movement_intent_system()");

	ecs.for_each_components<InputComponent, MovementIntent, MovementConfig>(
      [&](const Entity e, const InputComponent& ic, MovementIntent& intent, const MovementConfig& cfg){
      // --- Build local movement direction from input ---
      glm::vec3 input_dir(0.0f);

      if (ic.forward)	  input_dir.z -= 1.0f;
      if (ic.backward)  input_dir.z += 1.0f;
      if (ic.left)	    input_dir.x -= 1.0f;
      if (ic.right)	    input_dir.x += 1.0f;

      if (glm::dot(input_dir, input_dir) > 0.0f)
      input_dir = glm::normalize(input_dir);

      // log::system_info("movement_intent_system", "called for entity: {}", e.id);

      // --- Rotate movement into world-space based on where the camera is looking ---
      glm::vec3 forward = trans->forward();
      glm::vec3 right   = trans->right();

      forward.y = 0.0f;
      right.y = 0.0f;

      glm::vec3 wish_dir = right * input_dir.x + forward * input_dir.z;
      if (glm::dot(wish_dir, wish_dir) > 0.01f)
        wish_dir = glm::normalize(wish_dir);

      // --- Write intent ---
      intent.wish_dir = wish_dir;
      intent.jump     = ic.jump && cfg.can_jump;
      intent.sprint   = ic.sprint && cfg.can_run;
      intent.crouch   = ic.crouch && cfg.can_crouch;
      }
  );
}

void player_state_system(ECS& ecs)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
  ecs.for_each_components<PlayerState, MovementIntent, Collider>(
     [&](const Entity e, PlayerState& ps, const MovementIntent& intent, const Collider& col)
     {
     // Advance movement state
     ps.previous = ps.current;
     const bool moving = glm::dot(intent.wish_dir, intent.wish_dir);
     if (!col.is_on_ground)
     {
     ps.current = (ps.current == PlayerMovementState::Jumping)
     ? PlayerMovementState::Jumping
     : PlayerMovementState::Falling;
     return;
     }

     if (intent.crouch)
     ps.current = PlayerMovementState::Crouching;
     else if (intent.sprint && moving)
     ps.current = PlayerMovementState::Running;
     else if (moving)
     ps.current = PlayerMovementState::Walking;
     else
     ps.current = PlayerMovementState::Idle;
     }
  );
}

bool isCollidingAt(const glm::vec3& pos, const Collider& col, ChunkManager& chunkManager)
{
#if defined(TRACY_ENABLE)
    ZoneScopedN("isCollidingAt");
#endif
    const AABB box = col.get_AABB_at(pos);

    // Integer bounds (inclusive)
    const glm::ivec3 minBlock = glm::floor(box.min);
    const glm::ivec3 maxBlock = glm::floor(box.max - glm::vec3(1e-4f)); // Avoid off-by-one on exact edges

	for (int y = minBlock.y; y <= maxBlock.y; ++y) {
        for (int x = minBlock.x; x <= maxBlock.x; ++x) {
            for (int z = minBlock.z; z <= maxBlock.z; ++z) {
                const glm::ivec3 blockWorldPos(x, y, z);

                Chunk* chunk = chunkManager.getChunk(blockWorldPos);
                if (!chunk)
                    continue; // Treat missing chunks as air (or return true for "void" if preferred)

                const glm::ivec3 local = world_to_local(blockWorldPos);
                const Block::blocks& type = chunk->get_block_type(local.x, local.y, local.z);

                if (type != Block::blocks::AIR &&
                    type != Block::blocks::WATER &&
                    type != Block::blocks::LAVA) {
                    return true;
                }
            }
        }
    }
    return false;
}

void movement_physics_system(ECS& ecs, ChunkManager& chunkManager, float dt)
{
#if defined(TRACY_ENABLE)
    ZoneScopedN("movement_physics_system");
#endif

    ecs.for_each_components<MovementIntent, Velocity, PlayerState, Collider, MovementConfig, Transform>(
        [&](const Entity e, const MovementIntent& intent, Velocity& vel, const PlayerState& ps, Collider& col, const MovementConfig& cfg, Transform& trans) {
#if defined(TRACY_ENABLE)
            ZoneScopedN("Player Physics Tick");
#endif

            // --- Apply input to velocity ---
            float speed = 0.0f;
            switch (ps.current) {
                case PlayerMovementState::Walking:   speed = cfg.walk_speed;   break;
                case PlayerMovementState::Running:   speed = cfg.run_speed;    break;
                case PlayerMovementState::Crouching: speed = cfg.crouch_speed; break;
                case PlayerMovementState::Flying:    speed = cfg.fly_speed;    break;
                default:                             speed = cfg.walk_speed;   break;
            }

            vel.value.x = intent.wish_dir.x * speed;
            vel.value.z = intent.wish_dir.z * speed;

            if (ps.current == PlayerMovementState::Flying) {
                vel.value.y = intent.wish_dir.y * cfg.fly_speed;
            }

            // --- Jumping ---
            if (intent.jump && col.is_on_ground) {
                vel.value.y = std::sqrt(2.0f * GRAVITY * cfg.jump_height);
                col.is_on_ground = false;
            }

            // --- Gravity ---
            // if (!col.is_on_ground && !cfg.can_fly) {
            //     vel.value.y -= GRAVITY * dt;
            // }

            // --- Early out if stationary ---
            if (glm::all(glm::epsilonEqual(vel.value, glm::vec3(0.0f), 1e-4f))) {
                col.is_on_ground = isCollidingAt(trans.pos - glm::vec3(0.0f, 0.05f, 0.0f), col, chunkManager);
                // col.aabb = col.get_AABB_at(trans.pos);
                col.halfExtents = col.get_AABB_at(trans.pos).extent();
                return;
            }

            const glm::vec3 oldPos = trans.pos;
            // s = s_0 + v*t;
            glm::vec3 newPos = trans.pos + vel.value * dt;
            col.is_on_ground = false;

            // --- Axis-by-axis sweep (Y → X → Z) ---
            // Y-axis
            glm::vec3 testPos = { oldPos.x, newPos.y, oldPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.y = testPos.y;
            } else {
                trans.pos.y = oldPos.y;
                if (vel.value.y < 0.0f) {
                    vel.value.y = 0.0f;
                    col.is_on_ground = true;
                } else if (vel.value.y > 0.0f) {
                    vel.value.y = 0.0f;
                }
            }

            // X-axis
            testPos = { newPos.x, trans.pos.y, oldPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.x = testPos.x;
            } else {
                trans.pos.x = oldPos.x;
                vel.value.x = 0.0f;
            }

            // Z-axis
            testPos = { trans.pos.x, trans.pos.y, newPos.z };
            if (!isCollidingAt(testPos, col, chunkManager)) {
                trans.pos.z = testPos.z;
            } else {
                trans.pos.z = oldPos.z;
                vel.value.z = 0.0f;
            }

            // --- Final ground check (small downward ray) ---
            if (!col.is_on_ground) {
                col.is_on_ground = isCollidingAt(trans.pos - glm::vec3(0.0f, 0.05f, 0.0f), col, chunkManager);
            }

            // Sync AABB once at the end
            col.halfExtents = col.get_AABB_at(trans.pos).extent();
        });
}

void camera_system(ECS& ecs, frame_context& ctx)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

  log::system_info("camera_system", "");

}

static inline glm::vec4 extract_plane(const glm::mat4& m, int axis, float sign)
{
  return glm::vec4(
      m[0][3] + sign * m[0][axis],
      m[1][3] + sign * m[1][axis],
      m[2][3] + sign * m[2][axis],
      m[3][3] + sign * m[3][axis]
      );
}
enum frustum_axis : std::uint8_t {
  X = 0,
  Y = 1,
  Z = 2
};
void extractFrustumPlanes(const glm::mat4& vp, std::array<FrustumVolume::FrustumPlane,6>& planes) {
    // planes[0].equation = glm::vec4(VP[0][3] + VP[0][0], VP[1][3] + VP[1][0], VP[2][3] + VP[2][0], VP[3][3] + VP[3][0]);
    // planes[1].equation = glm::vec4(VP[0][3] - VP[0][0], VP[1][3] - VP[1][0], VP[2][3] - VP[2][0], VP[3][3] - VP[3][0]);
    // planes[2].equation = glm::vec4(VP[0][3] + VP[0][1], VP[1][3] + VP[1][1], VP[2][3] + VP[2][1], VP[3][3] + VP[3][1]);
    // planes[3].equation = glm::vec4(VP[0][3] - VP[0][1], VP[1][3] - VP[1][1], VP[2][3] - VP[2][1], VP[3][3] - VP[3][1]);
    // planes[4].equation = glm::vec4(VP[0][3] + VP[0][2], VP[1][3] + VP[1][2], VP[2][3] + VP[2][2], VP[3][3] + VP[3][2]);
    // planes[5].equation = glm::vec4(VP[0][3] - VP[0][2], VP[1][3] - VP[1][2], VP[2][3] - VP[2][2], VP[3][3] - VP[3][2]);

    planes[0].equation = extract_plane(vp, X, +1);
    planes[1].equation = extract_plane(vp, X, -1);
    planes[2].equation = extract_plane(vp, Y, +1);
    planes[3].equation = extract_plane(vp, Y, -1);
    planes[4].equation = extract_plane(vp, Z, +1);
    planes[5].equation = extract_plane(vp, Z, -1);

    for (auto& plane : planes) {
      assert(glm::length(glm::vec3(plane.equation)) > 0.0f);
      // float len = glm::length(glm::vec3(plane.equation));
      // if (len > 0.0f) plane.equation /= len;
      float inv_len = glm::inversesqrt(glm::dot(glm::vec3(plane.equation), glm::vec3(plane.equation)));
      plane.equation *= inv_len;
    }
}

void frustum_volume_system(ECS& ecs)
{

	ecs.for_each_components<Camera, Transform, FrustumVolume>(
			[&](const Entity e, const Camera& cam, const Transform& trans, FrustumVolume& fv) {
			extractFrustumPlanes(cam.projection_matrix() * cam.view_matrix(trans), fv.planes);
			});

}

void camera_temporal_system(ECS& ecs, const frame_context& ctx)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

  if (ecs.has_component<CameraTemporal>(ctx.active_camera)) {
    CameraTemporal* temp = ecs.get_component<CameraTemporal>(ctx.active_camera);
    temp->prev_view_proj = ctx.view_proj_matrix;
  }
}

void world_interaction_system(ECS& ecs, ChunkManager& manager) {
  // TODO: Implemnt this
}
