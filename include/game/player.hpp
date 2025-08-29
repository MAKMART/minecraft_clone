#pragma once
#include <memory>
#include <optional>
#include <GL/glew.h>
#include "core/defines.hpp"
#include "core/input_manager.hpp"
#include "chunk/chunk_manager.hpp"
#include "graphics/cube.hpp"
#include "core/aabb.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/velocity.hpp"
#include "game/ecs/components/player_state.hpp"
#include "game/ecs/components/player_mode.hpp"
#include "game/ecs/components/input.hpp"
#include "game/ecs/components/collider.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/camera.hpp"
#include "game/ecs/components/camera_controller.hpp"
#include "game/ecs/components/mouse_input.hpp"


class Player
{
      public:
	Player(ECS& ecs, glm::vec3 spawnPos, InputManager& _input)
	    : ecs(ecs), playerHeight(1.8f), input(_input), prevPlayerHeight(playerHeight), skinTexture(std::make_unique<Texture>(DEFAULT_SKIN_DIRECTORY, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST))
	{
		log::info("Initializing Player...");
		self = ecs.create_entity();

		// Create the player entity
		ecs.add_component(self, Transform{spawnPos});
		transform = ecs.get_component<Transform>(self);

		ecs.add_component(self, Velocity{});
		velocity = ecs.get_component<Velocity>(self);

		ecs.add_component(self, Collider{glm::vec3(ExtentX, playerHeight / 2.0f, ExtentY)});
		collider = ecs.get_component<Collider>(self);

		ecs.add_component(self, InputComponent{});
		input_comp = ecs.get_component<InputComponent>(self);

		ecs.add_component(self, PlayerState{});
		state = ecs.get_component<PlayerState>(self);

		ecs.add_component(self, PlayerMode{});
		mode = ecs.get_component<PlayerMode>(self);

		// Create the camera entity
		camera = ecs.create_entity();
		ecs.add_component(camera, Transform{spawnPos + glm::vec3(0.0f, eyeHeight, 0.0f)});
		ecs.add_component(camera, Camera{});
		ecs.add_component(camera, CameraController(self));
		ecs.add_component(camera, MouseInput{});

		// Other init stuff
		eyeHeight       = playerHeight * 0.9f;
		scaleFactor     = 0.076f;
		lastScaleFactor = scaleFactor;

		setupBodyParts();
		glCreateVertexArrays(1, &skinVAO);
		glBindVertexArray(skinVAO);
	}

	~Player()
	{
		glDeleteVertexArrays(1, &skinVAO);
	}

	enum ACTION { BREAK_BLOCK,
		      PLACE_BLOCK };

	const glm::vec3& getPos() const
	{
		return transform->pos;
	}
	const Entity& getCamera() const
	{
		return camera;
	}
	const glm::mat4& getModelMatrix() const
	{
		return modelMat;
	}

	// Non-const ref can be changed
	// If you'd not return it as a & the user would just modify its local copy of the type
	glm::vec3& getArmOffset()
	{
		return armOffset;
	};

	void update(float deltaTime, ChunkManager& chunkManager);
	void render(const Shader& shader);
	void loadSkin(const std::string& path);
	void loadSkin(const std::filesystem::path& path);

	bool is_on_ground() const
	{
		return collider->is_on_ground;
	}

	bool isRunning() const
	{
		return state->current == PlayerMovementState::Running;
	}

	bool isWalking() const
	{
		return state->current == PlayerMovementState::Walking;
	}

	bool isIdling() const
	{
		return state->current == PlayerMovementState::Idle;
	}

	bool isCrouching() const
	{
		return state->current == PlayerMovementState::Crouching;
	}

	bool isFlying() const
	{
		return state->current == PlayerMovementState::Flying;
	}

	bool isJumping() const
	{
		return state->current == PlayerMovementState::Jumping;
	}

	bool isSwimming() const
	{
		return state->current == PlayerMovementState::Swimming;
	}

	bool isFalling() const
	{
		return state->current == PlayerMovementState::Falling;
	}

	void processMouseInput(ChunkManager& chunkManager);
	void processKeyInput();
	void processMouseScroll(float yoffset);

	void setPos(glm::vec3 newPos);

	const glm::vec3& getVelocity() const
	{
		return velocity->value;
	}

	AABB getAABB() const
	{
		return collider->getBoundingBoxAt(getPos());
	}

	bool isInsidePlayerBoundingBox(const glm::vec3& checkPos) const;
	void breakBlock(ChunkManager& chunkManager);
	void placeBlock(ChunkManager& chunkManager);

	const char* getMode() const
	{
		switch (mode->mode) {
		case Type::SURVIVAL:
			return "SURVIVAL";
			break;
		case Type::CREATIVE:
			return "CREATIVE";
			break;
		case Type::SPECTATOR:
			return "SPECTATOR";
			break;
		default:
			return "UNHANDLED";
		}
	}

	const char* getMovementState() const
	{
		switch (state->current) {
		case PlayerMovementState::Idle:
			return "IDLE";
			break;
		case PlayerMovementState::Walking:
			return "WALKING";
			break;
		case PlayerMovementState::Running:
			return "RUNNNIG";
			break;
		case PlayerMovementState::Jumping:
			return "JUMPING";
			break;
		case PlayerMovementState::Crouching:
			return "CROUCHING";
			break;
		case PlayerMovementState::Flying:
			return "FLYING";
			break;
		case PlayerMovementState::Swimming:
			return "SWIMMING";
			break;
		case PlayerMovementState::Falling:
			return "FALLING";
			break;
		default:
			return "UNHANDLED";
		}
	}

	// --- Player Settings ---
	float        animationTime = 0.0f;
	float        scaleFactor;
	float        lastScaleFactor;
	float        walking_speed            = 3.6f;
	float        running_speed_increment  = 4.2f;
	float        running_speed            = walking_speed + running_speed_increment;
	float        swimming_speed           = 8.0f;
	float        flying_speed             = 17.5f;
	float        JUMP_FORCE               = 4.5f;
	const float  h                        = 1.252f; // Height of the jump of the player
	float        max_interaction_distance = 10.0f;
	unsigned int render_distance          = 5;

	float        playerHeight;
	float        prevPlayerHeight;
	float        eyeHeight;
	std::uint8_t selectedBlock = 1;

	// --- Player Flags ---
	bool isDamageable = false;

	bool canPlaceBlocks = true;
	bool canBreakBlocks = true;
	bool renderSkin     = false;
	bool canFly         = true;
	bool canWalk        = true;
	bool canRun         = true;
	bool canJump        = true;
	bool canCrouch      = true;

	bool hasInfiniteBlocks = false;

      private:
	ECS& ecs;
	Entity self;
	Entity camera;
	// -- Player Model ---
	struct BodyPart {
		std::unique_ptr<Cube> cube;
		glm::vec3             offset;
		glm::mat4             transform = glm::mat4(1.0f);
	};
	std::vector<BodyPart> bodyParts;

	glm::vec3 armOffset = glm::vec3(0.3f, -0.792f, -0.5f); // Right, down, forward

	std::unique_ptr<Texture> skinTexture;
	GLuint                   skinVAO;

	glm::mat4 modelMat;

	Transform*       transform;
	Velocity*        velocity;
	Collider*        collider;
	InputComponent*  input_comp;
	PlayerState*     state;
	PlayerMode*      mode;

	float ExtentX = 0.4f;
	float ExtentY = 0.4f;

	InputManager& input;

	void setupBodyParts();
};
