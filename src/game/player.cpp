#include "game/player.hpp"
#include "core/defines.hpp"
#include "core/camera_controller.hpp"
#include "graphics/cube.hpp"
#include "glm/ext/matrix_transform.hpp"
#include <memory>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include "glm/ext/quaternion_float.hpp"
#include "glm/trigonometric.hpp"
#include "core/logger.hpp"
#include "core/timer.hpp"
#if defined (TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif


Player::~Player() {
    glDeleteVertexArrays(1, &skinVAO);
}
void Player::setupBodyParts() {
    bodyParts.resize(6);
    
    // Base sizes
    glm::vec3 headSize = glm::vec3(8, 8, 8);
    glm::vec3 torsoSize = glm::vec3(8, 12, 4);
    glm::vec3 limbSize = glm::vec3(4, 12, 4);

    // Base offsets
    glm::vec3 headOffset = glm::vec3(0, 18, 0);
    glm::vec3 torsoOffset = glm::vec3(0, 6, 0);
    glm::vec3 rightArmOffset = glm::vec3(-6, 6, 0);
    glm::vec3 leftArmOffset = glm::vec3(6, 6, 0);
    glm::vec3 rightLegOffset = glm::vec3(-2, 0, 0);
    glm::vec3 leftLegOffset = glm::vec3(2, 0, 0);

 
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
void Player::loadSkin(const std::string &path) {
    skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
void Player::loadSkin(const std::filesystem::path &path) {
    skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
bool Player::isInsidePlayerBoundingBox(const glm::vec3 &checkPos) const {
    return collider->aabb.intersects({checkPos, checkPos + glm::vec3(1.0f)});
}
void Player::render(const Shader &shader) {
#if defined (TRACY_ENABLE)
	ZoneScoped;
#endif
    if (!skinTexture) {
        log::system_error("Player", "skinTexture is null!");
        return;
    }

    shader.use();
    shader.setMat4("view", camCtrl.getViewMatrix());
    shader.setMat4("projection", camCtrl.getProjectionMatrix());
    skinTexture->Bind(1);
    glBindVertexArray(skinVAO);

    glm::vec3 forward = camCtrl.getFront();

    glDisable(GL_CULL_FACE);

    if (camCtrl.isThirdPersonMode()) {
        // Flatten the camera direction to ignore Y (vertical) component for horizontal facing
        forward.y = 0.0f;
        forward = glm::normalize(forward);

        // Player's forward direction (initially facing -Z, or (0, 0, -1))
        glm::vec3 playerForward(0.0f, 0.0f, -1.0f);

        // Calculate the rotation angle around the Y-axis
        float dot = glm::dot(playerForward, forward);
        dot = glm::clamp(dot, -1.0f, 1.0f);                    // Ensure dot product is within valid range
        float angle = glm::degrees(std::acos(dot)); // Convert to degrees

        // Determine rotation direction (cross product for right-hand rule)
        glm::vec3 cross = glm::cross(playerForward, forward);
        if (cross.y < 0)
            angle = -angle; // Rotate clockwise if cross.y is negative

        glm::mat4 baseTransform = getModelMatrix();
        // Apply rotation around Y-axis to face the camera
        baseTransform *= glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

        for (const auto &part : bodyParts) {
            glm::mat4 transform = baseTransform * glm::translate(glm::mat4(1.0f), part.offset) * part.transform;
            transform = glm::translate(transform, glm::vec3(0, 0.5, 0));
            //log::system_info("Player", "Rendering body part with size: {}", glm::to_string(part.cube->getSize()));
            shader.setVec3("cubeSize", part.cube->getSize()); // Pass size_ to shader
            shader.setMat4("model", transform);
            part.cube->render(transform);
        }
    } else {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        const auto &rightArm = bodyParts[2]; // Right arm at index 2

        glm::mat4 armTransform = getModelMatrix();
        glm::mat4 viewRotation = glm::mat4(glm::vec4(camCtrl.getRight(), 0.0f), glm::vec4(camCtrl.getUp(), 0.0f), glm::vec4(-forward, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        armTransform = armTransform * viewRotation;

        armTransform = armTransform * glm::translate(glm::mat4(1.0f), armOffset);
        armTransform = armTransform * rightArm.transform;

        armTransform = glm::translate(armTransform, glm::vec3(0, 0.5, 0));

        shader.setMat4("model", armTransform);
        shader.setVec3("cubeSize", rightArm.cube->getSize()); // Pass size_ to shader
        //log::system_info("Player", "Rendering body part with size: {}", glm::to_string(rightArm.cube->getSize()));
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
void Player::update(float deltaTime, ChunkManager &chunkManager) {
#if defined (TRACY_ENABLE)
	ZoneScoped;
#endif
    animationTime += deltaTime;
    float speed = state->current == PlayerMovementState::Running ? 6.0f : 4.0f;
    float amplitude = state->current == PlayerMovementState::Running ? 0.7f : 0.5f;
    float swing = std::sin(animationTime * speed) * amplitude;

    if (collider->is_on_ground) {
        if (state->current == PlayerMovementState::Walking || state->current == PlayerMovementState::Running) {
            bodyParts[2].transform = glm::rotate(glm::mat4(1.0f), -swing, glm::vec3(1, 0, 0));
            bodyParts[3].transform = glm::rotate(glm::mat4(1.0f), swing, glm::vec3(1, 0, 0));
            bodyParts[4].transform = glm::rotate(glm::mat4(1.0f), swing, glm::vec3(1, 0, 0));
            bodyParts[5].transform = glm::rotate(glm::mat4(1.0f), -swing, glm::vec3(1, 0, 0));
        } else {
            // → reset transforms
            for (auto &part : bodyParts)
                part.transform = glm::mat4(1.0f);
        }
    } else {
        // In air
        float bend = 0.3f;
        bodyParts[4].transform = glm::rotate(glm::mat4(1.0f), bend, glm::vec3(1, 0, 0));
        bodyParts[5].transform = glm::rotate(glm::mat4(1.0f), bend, glm::vec3(1, 0, 0));
        bodyParts[2].transform = glm::rotate(glm::mat4(1.0f), -bend / 2, glm::vec3(1, 0, 0));
        bodyParts[3].transform = glm::rotate(glm::mat4(1.0f), -bend / 2, glm::vec3(1, 0, 0));
    }

    updateCamPos();

    if (camCtrl.isThirdPersonMode())
        modelMat = glm::translate(glm::mat4(1.0f), position->value + eyeHeight);
    else
        modelMat = glm::translate(glm::mat4(1.0f), position->value);

    camCtrl.update(deltaTime);  // Update the CameraController
}
void Player::setPos(glm::vec3 newPos) {
    position->value = newPos;
    updateCamPos();
}

void Player::processMouseInput(ChunkManager &chunkManager) {
    if (input.isMousePressed(ATTACK_BUTTON) && this->canBreakBlocks) {
        breakBlock(chunkManager);
    }
    if (input.isMousePressed(DEFENSE_BUTTON) && this->canPlaceBlocks) {
        placeBlock(chunkManager);
    }

}
void Player::processKeyInput() {
    if (input.isPressed(CAMERA_SWITCH_KEY))
        toggleCameraMode();

}
void Player::breakBlock(ChunkManager &chunkManager) {
    std::optional<glm::ivec3> hitBlock = raycastVoxel(chunkManager, camCtrl.getCurrentPosition(), camCtrl.getFront(), max_interaction_distance);

    if (!hitBlock.has_value())
        return;

    glm::ivec3 blockPos = hitBlock.value();

    //log::system_info("Player", "Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);

    chunkManager.updateBlock(blockPos, Block::blocks::AIR);
}
void Player::placeBlock(ChunkManager &chunkManager) {
    std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycastVoxelWithNormal(
        chunkManager, camCtrl.getCurrentPosition(), camCtrl.getFront(), max_interaction_distance);

    if (!hitResult.has_value())
        return;

    glm::ivec3 hitBlockPos = hitResult->first;
    glm::ivec3 normal = hitResult->second;

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

    //log::system_info("Player", "Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);

    chunkManager.updateBlock(placePos, static_cast<Block::blocks>(selectedBlock));
}

std::optional<glm::ivec3> Player::raycastVoxel(ChunkManager &chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance) {
    // Initialize voxel position (world-space)
    glm::ivec3 worldResult(
        static_cast<int>(std::floor(rayOrigin.x)),
        static_cast<int>(std::floor(rayOrigin.y)),
        static_cast<int>(std::floor(rayOrigin.z)));

    // Step direction for each axis
    glm::ivec3 step(
        (rayDirection.x >= 0) ? 1 : -1,
        (rayDirection.y >= 0) ? 1 : -1,
        (rayDirection.z >= 0) ? 1 : -1);

    // Compute tMax: distance until first voxel boundary is crossed
    glm::vec3 tMax(
        (rayDirection.x != 0.0f) ? ((step.x > 0 ? (worldResult.x + 1.0f - rayOrigin.x) : (rayOrigin.x - worldResult.x)) / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
        (rayDirection.y != 0.0f) ? ((step.y > 0 ? (worldResult.y + 1.0f - rayOrigin.y) : (rayOrigin.y - worldResult.y)) / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
        (rayDirection.z != 0.0f) ? ((step.z > 0 ? (worldResult.z + 1.0f - rayOrigin.z) : (rayOrigin.z - worldResult.z)) / std::abs(rayDirection.z)) : std::numeric_limits<float>::max());

    // Compute tDelta: how far to move in t to cross a voxel
    glm::vec3 tDelta(
        (rayDirection.x != 0.0f) ? (1.0f / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
        (rayDirection.y != 0.0f) ? (1.0f / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
        (rayDirection.z != 0.0f) ? (1.0f / std::abs(rayDirection.z)) : std::numeric_limits<float>::max());

    float t = 0.0f;
    while (t <= maxDistance) {
        Chunk* chunk = chunkManager.getChunk(worldResult);

        if (chunk) {
            glm::ivec3 localVoxelPos = Chunk::worldToLocal(worldResult);
            int blockIndex = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

            if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
                chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
                return worldResult; // Found a solid block
            }
        }

        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                worldResult.x += step.x;
                t = tMax.x;
                tMax.x += tDelta.x;
            } else {
                worldResult.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                worldResult.y += step.y;
                t = tMax.y;
                tMax.y += tDelta.y;
            } else {
                worldResult.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        }
    }

    return std::nullopt;
}

std::optional<std::pair<glm::ivec3, glm::ivec3>> Player::raycastVoxelWithNormal(
    ChunkManager &chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance) {
    glm::ivec3 worldResult = glm::floor(rayOrigin);
    glm::ivec3 step = glm::sign(rayDirection); // Step direction (-1 or +1)

    glm::vec3 tMax;
    glm::vec3 tDelta = glm::vec3(
        (rayDirection.x != 0.0f) ? (1.0f / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
        (rayDirection.y != 0.0f) ? (1.0f / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
        (rayDirection.z != 0.0f) ? (1.0f / std::abs(rayDirection.z)) : std::numeric_limits<float>::max());

    for (int i = 0; i < 3; i++) {
        if (rayDirection[i] > 0)
            tMax[i] = (worldResult[i] + 1 - rayOrigin[i]) * tDelta[i];
        else
            tMax[i] = (rayOrigin[i] - worldResult[i]) * tDelta[i];
    }

    float t = 0.0f;
    glm::ivec3 lastVoxel = worldResult;

    while (t < maxDistance) {
       Chunk* chunk = chunkManager.getChunk({worldResult.x, 0, worldResult.z});

        if (chunk) {
            glm::ivec3 localVoxelPos = Chunk::worldToLocal(worldResult);
            int blockIndex = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

            if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
                chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
                return std::make_pair(worldResult, worldResult - lastVoxel);
            }
        }

        lastVoxel = worldResult;

        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                worldResult.x += step.x;
                t = tMax.x;
                tMax.x += tDelta.x;
            } else {
                worldResult.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                worldResult.y += step.y;
                t = tMax.y;
                tMax.y += tDelta.y;
            } else {
                worldResult.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        }
    }

    return std::nullopt;
}
void Player::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    camCtrl.processMouseMovement(xoffset, yoffset, constrainPitch);
}
void Player::processMouseScroll(float yoffset) {
    float scroll_speed_multiplier = 1.0f;
    if (state->current == PlayerMovementState::Flying && mode->mode == Type::SPECTATOR) {
        flying_speed += yoffset;
        if (flying_speed <= 0)
            flying_speed = 0;
    } else if (!camCtrl.isThirdPersonMode()) {
        selectedBlock += (int)(yoffset * scroll_speed_multiplier);
        if (selectedBlock < 1)
            selectedBlock = 1;
        if (selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS))
            selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
    }

    // To change FOV
    //camCtrl.processMouseScroll(yoffset);
}
