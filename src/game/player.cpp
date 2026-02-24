#include "game/player.hpp"
#include "core/defines.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <optional>
#include "core/raycast.hpp"
#include "game/ecs/components/active_camera.hpp"
#include "game/ecs/components/temporal_camera.hpp"
#include "game/ecs/components/render_target.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

Player::Player(ECS& ecs, glm::vec3 spawnPos, int width, int height)
    : ecs(ecs), playerHeight(1.8f), input(InputManager::get())
{
	self = ecs.create_entity();
	ecs.emplace_component<Transform>(self, spawnPos);
	ecs.emplace_component<Velocity>(self);
	ecs.emplace_component<Collider>(self, glm::vec3(ExtentX, playerHeight / 2.0f, ExtentY));
	ecs.emplace_component<InputComponent>(self);
	ecs.emplace_component<MovementIntent>(self);
	ecs.emplace_component<MovementConfig>(self, MovementConfig{
			.can_jump = true,
			.can_walk = true,
			.can_run = true,
			.can_crouch = true,
			.can_fly = false,
			.jump_height = 1.252f,
			.walk_speed = 3.6f,
			.run_speed = 777.8f,
			.crouch_speed = 2.5f,
			.fly_speed = 17.5f
			});
	ecs.emplace_component<PlayerState>(self);
	ecs.emplace_component<PlayerMode>(self);

	eyeHeight = playerHeight * 0.9f;
	camera = ecs.create_entity();
	ecs.emplace_component<Transform>(camera);
	ecs.emplace_component<Camera>(camera);
	ecs.emplace_component<CameraController>(camera, CameraController(self, glm::vec3(0.0f, eyeHeight / 2.0f, 0.0f)));
	// Bro, you know that if you don't mark the camera as "Active" it won't render a thing :)
	ecs.emplace_component<ActiveCamera>(camera);
	ecs.emplace_component<FrustumVolume>(camera);
	ecs.emplace_component<CameraTemporal>(camera);
	ecs.emplace_component<RenderTarget>(camera, RenderTarget(width, height, {
			{ framebuffer_attachment_type::color, GL_RGBA16F }, // albedo
			//{ framebuffer_attachment_type::color, GL_RGBA16F }, // normal
			//{ framebuffer_attachment_type::color, GL_RG16F   }, // material
			{ framebuffer_attachment_type::depth, GL_DEPTH_COMPONENT24 }
			}));

}
Player::~Player() {}
void Player::update(float deltaTime)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	CameraController* ctrl = ecs.get_component<CameraController>(camera);

	if (ctrl->third_person)
		modelMat = glm::translate(glm::mat4(1.0f), getPos() + eyeHeight);
	else
		modelMat = glm::translate(glm::mat4(1.0f), getPos());
}
void Player::breakBlock(ChunkManager& chunkManager)
{
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


	std::optional<glm::ivec3> hitBlock = raycast::voxel(chunkManager, ray_origin, ray_dir, max_interaction_distance);
	if (hitBlock.has_value()) {
		glm::ivec3 blockPos = hitBlock.value();
		chunkManager.updateBlock(blockPos, Block::blocks::AIR);
	}

	// log::system_info("Player", "Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);

}
void Player::placeBlock(ChunkManager& chunkManager)
{
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

		glm::ivec3 localBlockPos = Chunk::world_to_local(placePos);

		if (chunkManager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z) != Block::blocks::AIR) {
			log::system_info("Player", "❌ Target block is NOT air! It's type: {}", Block::toString(chunkManager.getChunk(placePos)->get_block_type(localBlockPos.x, localBlockPos.y, localBlockPos.z)));
			return;
		}

		//log::system_info("Player", "Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);
		/*
		int radius = 2;
		for(int i = -radius; i < radius; i++) {
			for(int j = -radius; j < radius; j++) {
				for(int k = -radius; k < radius; k++) {
					chunkManager.updateBlock(placePos + glm::ivec3(i, j, k), static_cast<Block::blocks>(selectedBlock));
				}
			}

		}
		*/
		chunkManager.updateBlock(placePos, static_cast<Block::blocks>(selectedBlock));
	}
}
