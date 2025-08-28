#include "game/player.hpp"
#include "core/defines.hpp"
#include "graphics/cube.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <memory>
#include <optional>
#include "glm/trigonometric.hpp"
#include "core/raycast.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

void Player::setupBodyParts()
{
	bodyParts.resize(6);

	// Base sizes
	glm::vec3 headSize  = glm::vec3(8, 8, 8);
	glm::vec3 torsoSize = glm::vec3(8, 12, 4);
	glm::vec3 limbSize  = glm::vec3(4, 12, 4);

	// Base offsets
	glm::vec3 headOffset     = glm::vec3(0, 18, 0);
	glm::vec3 torsoOffset    = glm::vec3(0, 6, 0);
	glm::vec3 rightArmOffset = glm::vec3(-6, 6, 0);
	glm::vec3 leftArmOffset  = glm::vec3(6, 6, 0);
	glm::vec3 rightLegOffset = glm::vec3(-2, 0, 0);
	glm::vec3 leftLegOffset  = glm::vec3(2, 0, 0);

	// Apply scale factor
	headSize *= scaleFactor;
	torsoSize *= scaleFactor;
	limbSize *= scaleFactor;
	headOffset *= scaleFactor;
	torsoOffset *= scaleFactor;
	rightArmOffset *= scaleFactor;
	leftArmOffset *= scaleFactor;
	rightLegOffset *= scaleFactor;
	leftLegOffset *= scaleFactor;

	bodyParts[0] = {std::make_unique<Cube>(headSize, BodyPartType::HEAD), headOffset};
	bodyParts[1] = {std::make_unique<Cube>(torsoSize, BodyPartType::TORSO), torsoOffset};
	bodyParts[2] = {std::make_unique<Cube>(limbSize, BodyPartType::RIGHT_ARM), rightArmOffset};
	bodyParts[3] = {std::make_unique<Cube>(limbSize, BodyPartType::LEFT_ARM), leftArmOffset};
	bodyParts[4] = {std::make_unique<Cube>(limbSize, BodyPartType::RIGHT_LEG), rightLegOffset};
	bodyParts[5] = {std::make_unique<Cube>(limbSize, BodyPartType::LEFT_LEG), leftLegOffset};
}
void Player::loadSkin(const std::string& path)
{
	skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
void Player::loadSkin(const std::filesystem::path& path)
{
	skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
bool Player::isInsidePlayerBoundingBox(const glm::vec3& checkPos) const
{
	return collider->aabb.intersects({checkPos, checkPos + glm::vec3(1.0f)});
}
void Player::render(const Shader& shader, const Camera& cam, const CameraController& ctrl)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	if (!skinTexture) {
		log::system_error("Player", "skinTexture is null!");
		return;
	}

	shader.use();
	shader.setMat4("view", cam.viewMatrix);
	shader.setMat4("projection", cam.projectionMatrix);
	skinTexture->Bind(1);
	glBindVertexArray(skinVAO);

	glm::vec3 forward = cam.forward;

	glDisable(GL_CULL_FACE);

	if (ctrl.third_person) {
		// Flatten the camera direction to ignore Y (vertical) component for horizontal facing
		forward.y = 0.0f;
		forward   = glm::normalize(forward);

		// Player's forward direction (initially facing -Z, or (0, 0, -1))
		glm::vec3 playerForward(0.0f, 0.0f, -1.0f);

		// Calculate the rotation angle around the Y-axis
		float dot   = glm::dot(playerForward, forward);
		dot         = glm::clamp(dot, -1.0f, 1.0f); // Ensure dot product is within valid range
		float angle = glm::degrees(std::acos(dot)); // Convert to degrees

		// Determine rotation direction (cross product for right-hand rule)
		glm::vec3 cross = glm::cross(playerForward, forward);
		if (cross.y < 0)
			angle = -angle; // Rotate clockwise if cross.y is negative

		glm::mat4 baseTransform = getModelMatrix();
		// Apply rotation around Y-axis to face the camera
		baseTransform *= glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

		for (const auto& part : bodyParts) {
			glm::mat4 transform = baseTransform * glm::translate(glm::mat4(1.0f), part.offset) * part.transform;
			transform           = glm::translate(transform, glm::vec3(0, 0.5, 0));
			// log::system_info("Player", "Rendering body part with size: {}", glm::to_string(part.cube->getSize()));
			shader.setVec3("cubeSize", part.cube->getSize()); // Pass size_ to shader
			shader.setMat4("model", transform);
			part.cube->render(transform);
		}
	} else {
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		const auto& rightArm = bodyParts[2]; // Right arm at index 2

		glm::mat4 armTransform = getModelMatrix();
		glm::mat4 viewRotation = glm::mat4(glm::vec4(cam.right, 0.0f), glm::vec4(cam.up, 0.0f), glm::vec4(-forward, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		armTransform           = armTransform * viewRotation;

		armTransform = armTransform * glm::translate(glm::mat4(1.0f), armOffset);
		armTransform = armTransform * rightArm.transform;

		armTransform = glm::translate(armTransform, glm::vec3(0, 0.5, 0));

		shader.setMat4("model", armTransform);
		shader.setVec3("cubeSize", rightArm.cube->getSize()); // Pass size_ to shader
		// log::system_info("Player", "Rendering body part with size: {}", glm::to_string(rightArm.cube->getSize()));
		rightArm.cube->render(armTransform);
		if (BLENDING) {
			glEnable(GL_BLEND);
		}
		if (DEPTH_TEST) {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
		}
	}
	if (FACE_CULLING) {
		glEnable(GL_CULL_FACE);
	}
	skinTexture->Unbind(1);
}
void Player::update(float deltaTime, ChunkManager& chunkManager, bool is_third_person)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	animationTime += deltaTime;
	float speed     = state->current == PlayerMovementState::Running ? 6.0f : 4.0f;
	float amplitude = state->current == PlayerMovementState::Running ? 0.7f : 0.5f;
	float swing     = std::sin(animationTime * speed) * amplitude;

	if (collider->is_on_ground) {
		if (state->current == PlayerMovementState::Walking || state->current == PlayerMovementState::Running) {
			bodyParts[2].transform = glm::rotate(glm::mat4(1.0f), -swing, glm::vec3(1, 0, 0));
			bodyParts[3].transform = glm::rotate(glm::mat4(1.0f), swing, glm::vec3(1, 0, 0));
			bodyParts[4].transform = glm::rotate(glm::mat4(1.0f), swing, glm::vec3(1, 0, 0));
			bodyParts[5].transform = glm::rotate(glm::mat4(1.0f), -swing, glm::vec3(1, 0, 0));
		} else {
			// → reset transforms
			for (auto& part : bodyParts)
				part.transform = glm::mat4(1.0f);
		}
	} else {
		// In air
		float bend             = 0.3f;
		bodyParts[4].transform = glm::rotate(glm::mat4(1.0f), bend, glm::vec3(1, 0, 0));
		bodyParts[5].transform = glm::rotate(glm::mat4(1.0f), bend, glm::vec3(1, 0, 0));
		bodyParts[2].transform = glm::rotate(glm::mat4(1.0f), -bend / 2, glm::vec3(1, 0, 0));
		bodyParts[3].transform = glm::rotate(glm::mat4(1.0f), -bend / 2, glm::vec3(1, 0, 0));
	}


	if (is_third_person)
		modelMat = glm::translate(glm::mat4(1.0f), getPos() + eyeHeight);
	else
		modelMat = glm::translate(glm::mat4(1.0f), getPos());

}
void Player::setPos(glm::vec3 newPos)
{
	transform->pos = newPos;
}

void Player::processMouseInput(ChunkManager& chunkManager, const Camera& cam, const Transform& trans)
{
	if (input.isMousePressed(ATTACK_BUTTON) && this->canBreakBlocks) {
		breakBlock(chunkManager, cam, trans);
	}
	if (input.isMousePressed(DEFENSE_BUTTON) && this->canPlaceBlocks) {
		placeBlock(chunkManager, cam, trans);
	}
}
void Player::processKeyInput()
{

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
void Player::breakBlock(ChunkManager& chunkManager, const Camera& cam, const Transform& trans)
{
	std::optional<glm::ivec3> hitBlock = raycast::voxel(chunkManager, trans.pos, cam.forward, max_interaction_distance);

	if (!hitBlock.has_value())
		return;

	glm::ivec3 blockPos = hitBlock.value();

	// log::system_info("Player", "Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);

	chunkManager.updateBlock(blockPos, Block::blocks::AIR);
}
void Player::placeBlock(ChunkManager& chunkManager, const Camera& cam, const Transform& trans)
{
	std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycast::voxel_normals(
	    chunkManager, trans.pos, cam.forward, max_interaction_distance);

	if (!hitResult.has_value())
		return;

	glm::ivec3 hitBlockPos = hitResult->first;
	glm::ivec3 normal      = hitResult->second;

	glm::ivec3 placePos = hitBlockPos + (-normal);

	if (isInsidePlayerBoundingBox(placePos))
		return;

	Chunk* placeChunk = chunkManager.getChunk(placePos);
	if (!placeChunk) {
		return;
	}

	glm::ivec3 localBlockPos = Chunk::worldToLocal(placePos);

	int blockIndex = placeChunk->getBlockIndex(localBlockPos.x, localBlockPos.y, localBlockPos.z);
	if (blockIndex < 0 || static_cast<size_t>(blockIndex) >= placeChunk->getChunkData().size()) {
		log::system_info("Player", "❌ Invalid block index: {}", blockIndex);
		return;
	}

	if (placeChunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
		log::system_info("Player", "❌ Target block is NOT air! It's type: {}", Block::toString(placeChunk->getChunkData()[blockIndex].type));
		return;
	}

	// log::system_info("Player", "Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);

	chunkManager.updateBlock(placePos, static_cast<Block::blocks>(selectedBlock));
}
void Player::processMouseScroll(float yoffset, bool is_third_person)
{
	float scroll_speed_multiplier = 1.0f;
	if (state->current == PlayerMovementState::Flying && mode->mode == Type::SPECTATOR) {
		flying_speed += yoffset;
		if (flying_speed <= 0)
			flying_speed = 0;
	} else if (!is_third_person) {
		selectedBlock += (int)(yoffset * scroll_speed_multiplier);
		if (selectedBlock < 1)
			selectedBlock = 1;
		if (selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS))
			selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
	}

	// To change FOV
	// camCtrl.processMouseScroll(yoffset);
}
