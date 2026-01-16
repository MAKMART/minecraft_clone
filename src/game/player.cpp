#include "game/player.hpp"
#include "core/defines.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <optional>
#include "glm/trigonometric.hpp"
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
	log::info("Initializing Player...");

	// Create the player entity
	self = ecs.create_entity();

	ecs.add_component(self, Transform{spawnPos});
	transform = ecs.get_component<Transform>(self);

	ecs.add_component(self, Velocity{});
	velocity = ecs.get_component<Velocity>(self);

	ecs.add_component(self, Collider{glm::vec3(ExtentX, playerHeight / 2.0f, ExtentY)});
	collider = ecs.get_component<Collider>(self);

	ecs.add_component(self, InputComponent{});
	input_comp = ecs.get_component<InputComponent>(self);

	ecs.add_component(self, MovementIntent{});
	// Maybe add a local class reference here if you need

	ecs.add_component(self, MovementConfig{
			.can_jump = true,
			.can_walk = true,
			.can_run = true,
			.can_crouch = true,
			.can_fly = false,
			.jump_height = 1.252f,
			.walk_speed = 3.6f,
			.run_speed = 7.8f,
			.crouch_speed = 2.5f,
			.fly_speed = 17.5f
			});
	move_config = ecs.get_component<MovementConfig>(self);


	ecs.add_component(self, PlayerState{});
	state = ecs.get_component<PlayerState>(self);

	ecs.add_component(self, PlayerMode{});
	mode = ecs.get_component<PlayerMode>(self);

	// Other init stuff
	eyeHeight       = playerHeight * 0.9f;


	// Create the camera entity
	camera = ecs.create_entity();
	ecs.add_component(camera, Transform{});
	ecs.add_component(camera, Camera{});
	ecs.add_component(camera, CameraController{self, glm::vec3(0.0f, eyeHeight / 2.0f, 0.0f)});
	// Bro, you know that if you don't mark the camera as "Active" it won't render a thing :)
	ecs.add_component(camera, ActiveCamera{});
	ecs.add_component(camera, FrustumVolume{});
	ecs.add_component(camera, CameraTemporal{});
	ecs.add_component(camera, RenderTarget{width, height, {
			{ framebuffer_attachment_type::color, GL_RGBA16F }, // albedo
			//{ framebuffer_attachment_type::color, GL_RGBA16F }, // normal
			//{ framebuffer_attachment_type::color, GL_RG16F   }, // material
			{ framebuffer_attachment_type::depth, GL_DEPTH_COMPONENT24 }
			}});


}
Player::~Player()
{
}
bool Player::isInsidePlayerBoundingBox(const glm::vec3& checkPos) const
{
	return collider->aabb.intersects({checkPos, checkPos + glm::vec3(1.0f)});
}
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
void Player::setPos(glm::vec3 newPos)
{
	transform->pos = newPos;
}

void Player::processMouseInput(ChunkManager& chunkManager)
{
	if (input.isMousePressed(ATTACK_BUTTON) && this->canBreakBlocks) {
		breakBlock(chunkManager);
	}
	if (input.isMousePressed(DEFENSE_BUTTON) && this->canPlaceBlocks) {
		placeBlock(chunkManager);
	}
}
void Player::processKeyInput()
{

	if (input.isPressed(CAMERA_SWITCH_KEY)) {
		ecs.get_component<CameraController>(camera)->third_person = !ecs.get_component<CameraController>(camera)->third_person;
	}

	if (input.isPressed(SURVIVAL_MODE_KEY)) {
		state->current = PlayerMovementState::Walking;
	}

	if (input.isPressed(CREATIVE_MODE_KEY)) {
		state->current = PlayerMovementState::Flying;
	}

	if (input.isPressed(SPECTATOR_MODE_KEY)) {
		mode->mode = Type::SPECTATOR;
	}
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
		if (isInsidePlayerBoundingBox(placePos))
			return;

		glm::ivec3 localBlockPos = Chunk::worldToLocal(placePos);

		if (chunkManager.getChunk(placePos)->getBlockAt(localBlockPos.x, localBlockPos.y, localBlockPos.z).type != Block::blocks::AIR) {
			log::system_info("Player", "❌ Target block is NOT air! It's type: {}", Block::toString(chunkManager.getChunk(placePos)->getBlockAt(localBlockPos.x, localBlockPos.y, localBlockPos.z).type));
			return;
		}

		//log::system_info("Player", "Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);
		chunkManager.updateBlock(placePos, static_cast<Block::blocks>(selectedBlock));
	}
}
void Player::processMouseScroll(float yoffset)
{
	CameraController* ctrl                    = ecs.get_component<CameraController>(camera);
	float             scroll_speed_multiplier = 1.0f;
	if (state->current == PlayerMovementState::Flying && mode->mode == Type::SPECTATOR) {
		//flying_speed += yoffset;
		//if (flying_speed <= 0)
			//flying_speed = 0;
	} else if (ctrl->third_person) {
		selectedBlock += (int)(yoffset * scroll_speed_multiplier);
		if (selectedBlock < 1)
			selectedBlock = 1;
		if (selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS))
			selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
	}

	// To change FOV
	// camCtrl.processMouseScroll(yoffset);
}
