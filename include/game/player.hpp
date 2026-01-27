#pragma once
#include <cassert>
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
	Player(ECS& ecs, glm::vec3 spawnPos, int width, int height);
	~Player();
	enum ACTION { BREAK_BLOCK, PLACE_BLOCK };
	const glm::vec3& getPos() const
	{
		auto* t = ecs.get_component<Transform>(self);
		assert(t && "Player entity missing Transform component");
		return t->pos;
	}
	const glm::vec3& getVelocity() const
	{
		auto* v = ecs.get_component<Velocity>(self);
        assert(v && "Player entity missing Velocity component");
        return v->value;
	}
	const Entity& getCamera() const { return camera; }
	const Entity& getSelf() const { return self; }
	const glm::mat4& getModelMatrix() const { return modelMat; }
	void update(float deltaTime);

	bool is_on_ground() const
    {
        auto* c = ecs.get_component<Collider>(self);
        assert(c && "Player entity missing Collider component");
        return c->is_on_ground;
    }

	AABB getAABB() const 
	{
		auto* c = ecs.get_component<Collider>(self);
        assert(c && "Player entity missing Collider component");
		return c->getBoundingBoxAt(getPos());
	}

	bool isInsidePlayerBoundingBox(const glm::vec3& checkPos) const
	{
		auto* c = ecs.get_component<Collider>(self);
        assert(c && "Player entity missing Collider component");
		return c->aabb.intersects({checkPos, checkPos + glm::vec3(1.0f)});
	}
	void breakBlock(ChunkManager& chunkManager);
	void placeBlock(ChunkManager& chunkManager);

	const char* getMode() const
	{
		PlayerMode* mode = ecs.get_component<PlayerMode>(self);
		assert(mode && "Player entity missing PlayerMode component");
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
		PlayerState* state = ecs.get_component<PlayerState>(self);
		assert(state && "Player entity missing PlayerState component");
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
	float max_interaction_distance = 10.0f;
	unsigned int render_distance = 5;

	float playerHeight;
	float eyeHeight;
	std::uint8_t selectedBlock = 1;

	// --- Player Flags ---
	bool isDamageable = false;

	bool canPlaceBlocks = true;
	bool canBreakBlocks = true;
	bool renderSkin     = false;

	bool hasInfiniteBlocks = false;

      private:
	ECS&   ecs;
	Entity self;
	Entity camera;

	glm::mat4 modelMat;
	float ExtentX = 0.3f;
	float ExtentY = 0.3f;

	InputManager& input;
};
