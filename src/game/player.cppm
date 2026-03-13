module;
//#include <memory>
#include <cstdint>
export module player;

//import std;
import core;
import ecs;
import chunk_manager;
import glm;
import aabb;
import input_manager;

export class Player
{
	public:
		Player(ECS& ecs, glm::vec3 spawnPos, int width, int height);
		~Player() {}
		enum ACTION { BREAK_BLOCK, PLACE_BLOCK };
		const glm::vec3& getPos() const;
		const glm::vec3& getVelocity() const;
		const Entity& getCamera() const { return camera; }
		const Entity& getSelf() const { return self; }
		const glm::mat4& getModelMatrix() const { return modelMat; }

		void update(float deltaTime) noexcept;

		bool is_on_ground() const noexcept;
		AABB getAABB() const noexcept;
		bool isInsidePlayerBoundingBox(const glm::vec3& checkPos) const noexcept;

		void breakBlock(ChunkManager& chunkManager) noexcept;
		void placeBlock(ChunkManager& chunkManager) noexcept;

		const char* getMode() const noexcept;
		const char* getMovementState() const noexcept;

		// --- Player Settings ---
		float max_interaction_distance = 1000.0f; // 10
		unsigned int render_distance = 5u;

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
