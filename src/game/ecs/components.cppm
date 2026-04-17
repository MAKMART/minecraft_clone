module;
#include <gl.h>
export module ecs_components;

import glm;
import ecs;
import aabb;
import std;

export {

	// This is a component to handle which camera is currently active
	struct ActiveCamera {
    Entity target;
  };

	// This is just a tag component to handle whether a camera should be a debug kind
	struct DebugCamera {};

	struct Camera {
		explicit Camera(float _fov) : fov(_fov) {}
		Camera() {}

		float fov = 90.0f; // DEG

		// Be cautios of the underlying values' ratio as a 24-bit depth buffer will likely run out of percision
		float     near_plane   = 0.1f;
		float     far_plane    = 1000.0f;
		float     aspect_ratio = 16.0f / 9.0f;
		glm::mat4 viewMatrix{1.0f};
		glm::mat4 projectionMatrix{1.0f};

		glm::vec3 forward{0.0f, 0.0f, 1.0f};
		glm::vec3 up{0.0f, 1.0f, 0.0f};
		glm::vec3 right{1.0f, 0.0f, 0.0f};
	};
	struct CameraController {
		explicit CameraController(Entity _target) : target(_target) {}
		CameraController(Entity _target, const glm::vec3& _offset) : target(_target), offset(_offset) {}

		Entity    target;
		glm::vec3 offset{0.0f, 0.0f, 0.0f};
		bool      third_person   = false;
		float     orbit_distance = 5.0f;
		float     yaw            = 0.0f;
		float     pitch          = 0.0f;
		float     sensitivity    = 0.25f;
	};

	struct Collider {
		Collider() = default;
    Collider(const glm::vec3& pos, const glm::vec3& _halfExtents) : halfExtents(_halfExtents), aabb(AABB::fromCenterExtent(pos, _halfExtents)) {}
		explicit Collider(const glm::vec3& _halfExtents) : halfExtents(_halfExtents) {};

		AABB get_AABB_at(const glm::vec3& pos) const {
			return AABB::fromCenterExtent(pos, halfExtents);
		}

		glm::vec3 halfExtents;
		AABB      aabb;
		bool      solid = true; // optional: non-solid entities
		bool      is_on_ground = false;

	};

	struct DebugCameraController {
		float yaw = 0.0f;
		float pitch = 0.0f;
		float sensitivity = 0.25f;
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

	struct CameraTemporal {
		glm::mat4 prev_view_proj{1.0f};
	};

	struct Transform {
		Transform() = default;
		explicit Transform(const glm::vec3& pos) : pos(pos) {}
		Transform(const glm::vec3& pos, const glm::quat& quat) : pos(pos), rot(quat) {}
		Transform(const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale) : pos(pos), rot(quat), scale(scale) {}

		glm::vec3 pos{0.0f};
		glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity
		glm::vec3 scale{1.0f};
		Entity    parent; // invalid if no parent
	};
}
