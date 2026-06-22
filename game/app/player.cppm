export module player;

import std;
import ecs;

export struct Player
{
	public:
		Player() = default;
		~Player() {}
		enum ACTION { BREAK_BLOCK, PLACE_BLOCK };

		Entity self;
		Entity camera;

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

};

/*
bool Player::isInsidePlayerBoundingBox(const glm::vec3& checkPos) const noexcept
{
	auto* c = ecs.get_component<Collider>(self);
	assert(c && "Player entity missing Collider component");
	return c->aabb.intersects({checkPos, checkPos + glm::vec3(1.0f)});
}
void Player::breakBlock(ChunkManager& chunkManager) noexcept
{
	Timer block_break("block_breaking");
	Camera* cam = ecs.get_component<Camera>(camera);
	Transform* trans = ecs.get_component<Transform>(camera);

	// projection matrix (perspective)
	glm::mat4 projection = cam->projectionMatrix;
	// view matrix (world -> camera)
	glm::mat4 view = cam->viewMatrix;
	glm::vec4 clip = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

	glm::vec4 view_space = glm::inverse(projection) * clip;
	view_space.z = -1.0f; // forward direction
	view_space.w = 0.0f;  // this is a direction, not a position

	glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(view) * view_space));


	std::optional<glm::ivec3> hitBlock = raycast::voxel(chunkManager, trans->pos, ray_dir, max_interaction_distance);
	if (hitBlock.has_value()) {
		chunkManager.updateBlock(hitBlock.value(), Block::blocks::AIR);
	}

	// log::system_info("Player", "Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);

}
void Player::placeBlock(ChunkManager& chunkManager) noexcept
{
	Timer block_place("block_placing");
	Camera* cam = ecs.get_component<Camera>(camera);
	Transform* trans = ecs.get_component<Transform>(camera);

	// projection matrix (perspective)
	glm::mat4 projection = cam->projectionMatrix;
	// view matrix (world -> camera)
	glm::mat4 view = cam->viewMatrix;
	glm::vec4 clip = glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);

	glm::vec4 view_space = glm::inverse(projection) * clip;
	view_space.z = -1.0f; // forward direction
	view_space.w = 0.0f;  // this is a direction, not a position

	glm::vec3 ray_dir = glm::normalize(glm::vec3(glm::inverse(view) * view_space));
	glm::vec3 ray_origin = trans->pos; // start at camera position

	std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycast::voxel_normals(chunkManager, ray_origin, ray_dir, max_interaction_distance);

	if (hitResult.has_value()) {
		glm::ivec3 hitBlockPos = hitResult->first;
		glm::ivec3 normal      = hitResult->second;
		glm::ivec3 placePos = hitBlockPos + (-normal);
		if (isInsidePlayerBoundingBox(placePos) && !Block::isLiquid(static_cast<Block::blocks>(selectedBlock)))
			return;

		glm::ivec3 localBlockPos = world_to_local(placePos);

		if (chunkManager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z) != Block::blocks::AIR) {
			log::system_info("Player", "❌ Target block is NOT air! It's type: {}", Block::toString(chunkManager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z)));
			return;
		}

		//log::system_info("Player", "Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);
		   // int radius = 2;
		   // for(int i = -radius; i < radius; i++) {
		   // for(int j = -radius; j < radius; j++) {
		   // for(int k = -radius; k < radius; k++) {
		   // chunkManager.updateBlock(placePos + glm::ivec3(i, j, k), static_cast<Block::blocks>(selectedBlock));
		   // }
		   // }
		   //
		   // }
		chunkManager.updateBlock(placePos, static_cast<Block::blocks>(selectedBlock));
	}
}

*/
