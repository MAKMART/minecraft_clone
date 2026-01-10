#pragma once
#include <memory>
#include <optional>
#include <glad/glad.h>
#include "core/defines.hpp"
#include "core/input_manager.hpp"
#include "chunk/chunk_manager.hpp"
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
#include "game/ecs/components/movement_intent.hpp"
#include "game/ecs/components/movement_config.hpp"

class Player
{
      public:
	Player(ECS& ecs, glm::vec3 spawnPos);
	~Player();

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
	const Entity& getSelf() const
	{
		return self;
	}
	const glm::mat4& getModelMatrix() const
	{
		return modelMat;
	}

	void update(float deltaTime);

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
	float        max_interaction_distance = 10.0f;
	unsigned int render_distance          = 1;

	float        playerHeight;
	float        eyeHeight;
	std::uint8_t selectedBlock = 1;

	// --- Player Flags ---
	bool isDamageable = false;

	bool canPlaceBlocks = true;
	bool canBreakBlocks = true;
	bool renderSkin     = false;

	bool hasInfiniteBlocks = false;

	MovementConfig* getMovementConfig() const {
		return move_config;
	}

      private:
	ECS&   ecs;
	Entity self;
	Entity camera;
	glm::mat4 modelMat;

	MovementConfig* move_config;
	Transform*      transform;
	Velocity*       velocity;
	Collider*       collider;
	InputComponent* input_comp;
	PlayerState*    state;
	PlayerMode*     mode;

	float ExtentX = 0.4f;
	float ExtentY = 0.4f;

	InputManager& input;

};
