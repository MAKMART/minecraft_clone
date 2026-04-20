module;
#include <gl.h>
#include <cassert>
export module ecs_components;

import std;
import core;
import glm;
import ecs;
import aabb;

// ECS stores state, not derived data (unless explicitly cached)
// Components should store state, not duplicated or derived results.
// Because duplicated data = desync risk.
export {
	// This is just a tag component to handle whether a camera should be a debug kind
	struct DebugCamera {};

  // pos = final offset
  // rot = orientation of axes
  // scale = stretching of those axes
	struct Transform {
		Transform() = default;
		explicit Transform(const glm::vec3& pos) : pos(pos) {}
		Transform(const glm::vec3& pos, const glm::quat& quat) : pos(pos), rot(quat) {}
		Transform(const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale) : pos(pos), rot(quat) {
      set_scale(scale);
    }

    // LOCAL SPACE UTILITY METHODS
    glm::vec3 forward() const noexcept {
      return rot * coord::WORLD_FORWARD;
    }

    glm::vec3 up() const noexcept {
      return rot * coord::WORLD_UP;
    }

    glm::vec3 right() const noexcept {
      return rot * coord::WORLD_RIGHT;
    }

    glm::mat4 local_matrix() const noexcept {
      glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
      glm::mat4 R = glm::mat4_cast(rot);
      glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);

      return T * R * S;
    }

    glm::mat4 inverse_local_matrix() const noexcept {
      return glm::inverse(local_matrix());
    }

    void set_scale(glm::vec3 _scale) noexcept {
      assert(_scale.x > 0 && _scale.y > 0 && _scale.z > 0);
      scale = _scale;
    }

    glm::vec3 get_scale() const noexcept { return scale; }

		glm::vec3 pos{0.0f};
		glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity


    // TODO: Add hierarchical transform system later if you want
		// Entity    parent; // invalid if no parent
    private:
		glm::vec3 scale{1.0f};
	};

	struct Camera {
		explicit Camera(float _fov) : fov(_fov) {}
		Camera() {}

		float fov = 90.0f; // DEG

		// Be cautios of the underlying values' ratio as a 24-bit depth buffer will likely run out of percision
    float near_plane   = 0.1f;
    float far_plane    = 1000.0f;
    float aspect_ratio = 16.0f / 9.0f;

    glm::mat4 view_matrix(const Transform& t) const noexcept {
      return glm::mat4_cast(glm::conjugate(t.rot)) * glm::translate(glm::mat4(1.0f), -t.pos);
    }
    glm::mat4 projection_matrix() const noexcept {
      // Note that we use RH_ZO because we use reverse-Z depth,
      // if you want to change to forward-Z just change to RH_NO and switch the far with the near plane and vice-versa
      return glm::perspectiveRH_ZO( glm::radians(fov), aspect_ratio, far_plane, near_plane);
    }
	};

  // Enables rotating camera behavior
  struct CameraLook {
    float yaw = 0.0f;
    float pitch = 0.0f;
    float sensitivity = 0.25f;
  };

	struct CameraController {
		explicit CameraController(Entity _target) : target(_target) {}
		CameraController(Entity _target, const glm::vec3& _offset) : target(_target), offset(_offset) {}

		Entity    target;
		glm::vec3 offset{0.0f, 0.0f, 0.0f};
		bool      third_person   = false;
		float     orbit_distance = 5.0f;
	};

	struct CameraTemporal {
		glm::mat4 prev_view_proj{1.0f};
	};

	struct Collider {
		Collider() = default;
		explicit Collider(glm::vec3 _halfExtents) : halfExtents(_halfExtents) {};

		AABB get_AABB_at(glm::vec3 pos) const { return AABB::fromCenterExtent(pos, halfExtents); }

		glm::vec3 halfExtents{1.0f};
		bool      solid = true; // optional: non-solid entities
		bool      is_on_ground = false;

	};

	struct FrustumVolume {
		struct FrustumPlane {
			glm::vec4 equation; // ax + by + cz + d = 0

			glm::vec3 normal() const noexcept { return glm::vec3(equation); }
			inline float offset() const noexcept { return equation.w; }
		};
		std::array<FrustumPlane, 6> planes;
	};

	struct Health {
		int current = 20;
		int max     = 20;

		Health() = default;
		explicit Health(int maxHP) : current(maxHP), max(maxHP) {}
	};

	struct InputComponent {
		// All possible movement directions that can be controlled by an input device
		bool forward  = false;
		bool backward = false;
		bool left     = false;
		bool right    = false;
		bool jump     = false;
		bool sprint   = false;
		bool crouch   = false;

		glm::vec2 mouse_delta{0.0f};
		glm::vec2 scroll_delta{0.0f};

		bool is_primary_pressed   = false;
		bool is_ternary_pressed   = false;
		bool is_secondary_pressed = false;
	};

	struct MovementConfig {
		bool can_jump = true;
		bool can_walk = true;
		bool can_run = true;
		bool can_crouch = true;
		bool can_fly = false;
		float jump_height = 1.8f;
		float walk_speed = 5.0f;
		float run_speed = 7.5f;
		float crouch_speed = 2.5f;
		float fly_speed = 10.0f;
	};

	struct MovementIntent {
		glm::vec3 wish_dir = glm::vec3(0.0f);
		bool jump          = false;
		bool sprint        = false;
		bool crouch        = false;
	};

	enum ModeType : std::uint8_t { SURVIVAL, CREATIVE, SPECTATOR };
	struct PlayerMode {
		explicit PlayerMode(ModeType starting_mode) : mode(starting_mode) {}
		PlayerMode() = default;
		ModeType mode    = SURVIVAL;
	};

	enum class PlayerMovementState {
		Idle,
		Walking,
		Running,
		Crouching,
		Jumping,
		Swimming,
		Flying,
		Falling,
	};

	struct PlayerState {
		PlayerMovementState current    = PlayerMovementState::Idle;
		PlayerMovementState previous   = PlayerMovementState::Idle;
		bool isOnGround = true;
	};

	struct Position {
		glm::vec3 value{0.0f};
		Position() = default;
		Position(float x, float y, float z) : value(x, y, z) {}
		explicit Position(const glm::vec3& pos) : value(pos) {}
	};

	struct Velocity {
		glm::vec3 value{0.0f};
		Velocity() = default;
		Velocity(float x, float y, float z) : value(x, y, z) {}
		explicit Velocity(const glm::vec3& vel) : value(vel) {}
	};

	enum class extent_mode {
		fixed,
		follow_viewport
	};

	enum class framebuffer_attachment_type {
		color,
		depth,
		depth_stencil
	};

	struct framebuffer_attachment_desc {
		framebuffer_attachment_type type;
		GLenum internal_format;
	};
	struct RenderTarget {
		public:
			RenderTarget(int width, int height, std::initializer_list<framebuffer_attachment_desc> desc)
        : width(width), height(height), mode(extent_mode::fixed), attachments(desc) {}
      RenderTarget(std::initializer_list<framebuffer_attachment_desc> desc)
        : attachments(desc) {}
			int width  = 0;
			int height = 0;
			extent_mode mode = extent_mode::follow_viewport;
			std::vector<framebuffer_attachment_desc> attachments;
	};

}
