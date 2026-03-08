module;
#include <glad/glad.h>
#include <array>               // std::array<FrustumPlane, 6>
#include <cstdint>             // UINT32_MAX (used in CameraController + Transform)
#include <initializer_list>    // std::initializer_list in RenderTarget ctor
#include <vector>              // std::vector in RenderTarget

export module ecs_components;

import glm;
import ecs;
import aabb;

// This is just a tag component to handle whether a camera should be the main one which it's matrices should be used for rendering the final image to the framebuffer
export struct ActiveCamera {};

export struct Camera {
	explicit Camera(float _fov) : fov(_fov) {}
	Camera() {}

	float     fov          = 90.0f; // DEG
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

export struct CameraController {

	explicit CameraController(Entity _target) : target(_target) {}
	CameraController(Entity _target, const glm::vec3& _offset) : target(_target), offset(_offset) {}

	Entity    target{UINT32_MAX};
	glm::vec3 offset{0.0f, 0.0f, 0.0f};
	bool      third_person   = false;
	float     orbit_distance = 5.0f;
	float     yaw            = 0.0f;
	float     pitch          = 0.0f;
	float     sensitivity    = 0.25f;
};

export struct Collider {
	Collider() = default;
	Collider(const glm::vec3& min, const glm::vec3& max) : aabb(min, max) {}
	explicit Collider(const glm::vec3& _halfExtents) : halfExtents(_halfExtents) {};

	AABB getBoundingBoxAt(const glm::vec3& pos) const {
		return AABB::fromCenterExtent(pos, halfExtents);
	}

	glm::vec3 halfExtents;
	AABB      aabb;
	bool      solid = true; // optional: non-solid entities
	bool      is_on_ground = false;

};

export struct DebugCamera {};

export struct DebugCameraController {
	float yaw = 0.0f;
	float pitch = 0.0f;
	float sensitivity = 0.25f;
};

export struct FrustumVolume {
	struct FrustumPlane {
		glm::vec4 equation; // ax + by + cz + d = 0

		glm::vec3 normal() const
		{
			return glm::vec3(equation);
		}
		float offset() const
		{
			return equation.w;
		}
	};
	std::array<FrustumPlane, 6> planes;
};

export struct Health {
	int current = 20;
	int max     = 20;

	Health() = default;
	explicit Health(int maxHP) : current(maxHP), max(maxHP)
	{
	}
};

export struct InputComponent {
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

export struct MovementConfig {
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

export struct MovementIntent {
    glm::vec3 wish_dir = glm::vec3(0.0f);
    bool jump          = false;
    bool sprint        = false;
    bool crouch        = false;
};

export enum Type { SURVIVAL, CREATIVE, SPECTATOR };
export struct PlayerMode {
	explicit PlayerMode(Type starting_mode) : mode(starting_mode)
	{
	}
	PlayerMode() = default;
	Type mode    = SURVIVAL;
};

export enum class PlayerMovementState {
	Idle,
		Walking,
		Running,
		Crouching,
		Jumping,
		Swimming,
		Flying,
		Falling,
};

export struct PlayerState {
	PlayerMovementState current    = PlayerMovementState::Idle;
	PlayerMovementState previous   = PlayerMovementState::Idle;
	bool                isOnGround = true;
};

export struct Position {
	glm::vec3 value{0.0f};
	Position() = default;
	Position(float x, float y, float z) : value(x, y, z)
	{
	}
	explicit Position(const glm::vec3& pos) : value(pos)
	{
	}
};


export enum class extent_mode {
	fixed,
	follow_viewport
};

export enum class framebuffer_attachment_type {
	color,
	depth,
	depth_stencil
};

export struct framebuffer_attachment_desc {
	framebuffer_attachment_type type;
	GLenum internal_format;
};
export struct RenderTarget {
	public:
		RenderTarget(int width, int height, std::initializer_list<framebuffer_attachment_desc> desc) : width(width), height(height), attachments(desc) {}
		RenderTarget(int width, int height, extent_mode mode, std::initializer_list<framebuffer_attachment_desc> desc) : width(width), height(height), mode(mode), attachments(desc) {}
		int    width  = 0;
		int    height = 0;
		extent_mode mode = extent_mode::follow_viewport;
		std::vector<framebuffer_attachment_desc> attachments;
};

export struct CameraTemporal {
	glm::mat4 prev_view_proj{1.0f};
	bool      first_frame = true;
};

export struct Transform {
	Transform() = default;
	explicit Transform(const glm::vec3& pos) : pos(pos) {}
	Transform(const glm::vec3& pos, const glm::quat& quat) : pos(pos), rot(quat) {}
	Transform(const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale) : pos(pos), rot(quat), scale(scale) {}

	glm::vec3 pos{0.0f};
	glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // identity
	glm::vec3 scale{1.0f};
	Entity    parent = {UINT32_MAX}; // invalid if no parent
};

export struct Velocity {
	glm::vec3 value{0.0f};
	Velocity() = default;
	Velocity(float x, float y, float z) : value(x, y, z) {}
	explicit Velocity(const glm::vec3& vel) : value(vel) {}
};
