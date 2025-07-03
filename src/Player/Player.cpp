#include "Player.h"
#include "AABBDebugDrawer.h"
#include "PlayerState.h" // Full definition of PlayerState
#include "PlayerMode.h"  // Full definition of PlayerMode
#include "FlyingState.h"
#include "WalkingState.h"
#include "SurvivalMode.h"
#include "CreativeMode.h"
#include "SpectatorMode.h"
#include "defines.h"
#include "glm/ext/matrix_transform.hpp"
#include <memory>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include "logger.hpp"

// Constructor
Player::Player(glm::vec3 spawnPos, GLFWwindow *window)
    : _camera(new Camera(spawnPos)), prevPosition(spawnPos), velocity(0.0f), animationTime(0.0f), scaleFactor(0.076f), prevPlayerHeight(playerHeight) {
    eyeHeight = playerHeight * 0.9f;
    lastScaleFactor = scaleFactor;
    input = std::make_unique<InputManager>(window);
    skinTexture = std::make_unique<Texture>(DEFAULT_SKIN_DIRECTORY, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST); // Default skin
    position = glm::vec3(spawnPos.x, spawnPos.y - playerHeight, spawnPos.z);
    setupBodyParts();
    currentMode = std::make_unique<SurvivalMode>(); // STARTING MODE DEFAULT
    currentMode->enterMode(*this);                  // Sets initial state via mode
    updateBoundingBox();
    updateCameraPosition();
}
// Toggle between first-person and third-person
void Player::toggleCameraMode(void) {
    if (isThirdPerson) {
        // Switch to first-person
        _camera->SwitchToFirstPerson(position + glm::vec3(0.0f, eyeHeight, 0.0f));
    } else {
        // Switch to third-person, preserving the current first-person orientation
        _camera->SwitchToThirdPerson(position + glm::vec3(0.0f, eyeHeight, 0.0f), std::nullopt);
    }
    isThirdPerson = !isThirdPerson;
}
void Player::setupBodyParts(void) {
    bodyParts.resize(6);

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

    bodyParts[0] = {std::make_unique<Cube>(headSize, BodyPartType::HEAD), headOffset, glm::mat4(1.0f)};
    bodyParts[1] = {std::make_unique<Cube>(torsoSize, BodyPartType::TORSO), torsoOffset, glm::mat4(1.0f)};
    bodyParts[2] = {std::make_unique<Cube>(limbSize, BodyPartType::RIGHT_ARM), rightArmOffset, glm::mat4(1.0f)};
    bodyParts[3] = {std::make_unique<Cube>(limbSize, BodyPartType::LEFT_ARM), leftArmOffset, glm::mat4(1.0f)};
    bodyParts[4] = {std::make_unique<Cube>(limbSize, BodyPartType::RIGHT_LEG), rightLegOffset, glm::mat4(1.0f)};
    bodyParts[5] = {std::make_unique<Cube>(limbSize, BodyPartType::LEFT_LEG), leftLegOffset, glm::mat4(1.0f)};
}
void Player::loadSkin(const std::string &path) {
    skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
void Player::loadSkin(const std::filesystem::path &path) {
    skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
void Player::changeMode(std::unique_ptr<PlayerMode> newMode) {
    if (newMode) {
        if (currentMode)
            currentMode->exitMode(*this);
        currentMode = std::move(newMode);
        currentMode->enterMode(*this);
    }
}

// Change the state by moving a new state into currentState
void Player::changeState(std::unique_ptr<PlayerState> newState) {
    if (newState) {
        if (currentState)
            currentState->exitState(*this);
        currentState = std::move(newState);
        currentState->enterState(*this);
    }
}

// Destructor
Player::~Player(void) {
    delete _camera;
}
bool Player::isCollidingAt(const glm::vec3 &pos, ChunkManager &chunkManager) {
    AABB box = getBoundingBoxAt(pos);

    glm::ivec3 blockMin = glm::floor(box.min);
    glm::ivec3 blockMax = glm::ceil(box.max) - glm::vec3(1); // inclusive range fix!

    blockMin.y = std::max(blockMin.y, 0);
    blockMax.y = std::clamp(blockMax.y, 0, (int)chunkSize.y - 1);

    for (int by = blockMin.y; by <= blockMax.y; ++by) {
        for (int bx = blockMin.x; bx <= blockMax.x; ++bx) {
            for (int bz = blockMin.z; bz <= blockMax.z; ++bz) {

                glm::ivec3 blockWorldPos(bx, by, bz);
                glm::ivec3 chunkCoords = Chunk::worldToChunk(blockWorldPos, chunkSize);

                std::shared_ptr<Chunk> chunk = chunkManager.getChunk(blockWorldPos);
                if (!chunk) return true; // treat unloaded chunks as solid

                glm::ivec3 local = blockWorldPos - chunkCoords * chunkSize;
                glm::vec3 worldPos = glm::vec3(blockWorldPos);
                const Block &block = chunk->getBlockAt(local.x, local.y, local.z);

                //getAABBDebugDrawer().addAABB({worldPos, worldPos + glm::vec3(1.0f)}, glm::vec3(1.0f - 0.5f, 0, 0));
                if (block.type != Block::blocks::AIR) {
#if defined (DEBUG)
                    getAABBDebugDrawer().addAABB({worldPos, worldPos + glm::vec3(1.0f)}, glm::vec3(1, 0, 0));
#endif
                    return true;
                }
            }
        }
    }
   return false;
}


bool Player::isInsidePlayerBoundingBox(const glm::vec3 &checkPos) const {
    return aabb.intersects({checkPos, checkPos + glm::vec3(1.0f)});
}

void Player::handleCollisions(glm::vec3 &newPosition, glm::vec3 &velocity,
                              const glm::vec3 &oldPosition, ChunkManager &chunkManager) {
    glm::vec3 testPos = newPosition;
    isOnGround = false;

    // Try full movement
    if (!isCollidingAt(testPos, chunkManager)) {
        newPosition = testPos;
        goto groundCheck;
    }

    // Y-axis first
    testPos = {oldPosition.x, newPosition.y, oldPosition.z};
    if (!isCollidingAt(testPos, chunkManager)) {
        newPosition.y = testPos.y;
    } else {
        newPosition.y = oldPosition.y;
        velocity.y = 0;
        if (velocity.y < 0) isOnGround = true;
    }

    // X-axis
    testPos = {newPosition.x, newPosition.y, oldPosition.z};
    if (!isCollidingAt(testPos, chunkManager)) {
        newPosition.x = testPos.x;
    } else {
        newPosition.x = oldPosition.x;
        velocity.x = 0;
    }

    // Z-axis
    testPos = {oldPosition.x, newPosition.y, newPosition.z};
    if (!isCollidingAt(testPos, chunkManager)) {
        newPosition.z = testPos.z;
    } else {
        newPosition.z = oldPosition.z;
        velocity.z = 0;
    }

groundCheck:
    // Only do ground check when vertical velocity is zero (e.g. standing on block)
    if (velocity.y == 0.0f) {
        glm::vec3 groundCheckPos = newPosition;
        groundCheckPos.y -= 0.001f;
        if (isCollidingAt(groundCheckPos, chunkManager))
            isOnGround = true;
    }
}

void Player::render(unsigned int shaderProgram) {
    if (!skinTexture) {
        throw std::runtime_error("Error: skinTexture is null!");
        return;
    }

    skinTexture->Bind(1);

    if (isThirdPerson) {
        // Calculate the total height of the player model (torso + head, scaled)
        // float modelHeight = (torsoSize.y + headSize.y) * scaleFactor;  // Total height in Minecraft units, scaled

        // Start with the player's position (feet at position.y)
        glm::mat4 baseTransform = glm::translate(glm::mat4(1.0f), position);

        // Rotate the player to face the camera's Front direction
        glm::vec3 cameraDirection = _camera->getFront();
        // Flatten the camera direction to ignore Y (vertical) component for horizontal facing
        cameraDirection.y = 0.0f;
        cameraDirection = glm::normalize(cameraDirection);

        // Player's forward direction (initially facing -Z, or (0, 0, -1))
        glm::vec3 playerForward(0.0f, 0.0f, -1.0f);

        // Calculate the rotation angle around the Y-axis
        float dot = glm::dot(playerForward, cameraDirection);
        dot = glm::clamp(dot, -1.0f, 1.0f);                    // Ensure dot product is within valid range
        float angle = acos(dot) * (180.0f / std::numbers::pi); // Convert to degrees

        // Determine rotation direction (cross product for right-hand rule)
        glm::vec3 cross = glm::cross(playerForward, cameraDirection);
        if (cross.y < 0)
            angle = -angle; // Rotate clockwise if cross.y is negative

        // Apply rotation around Y-axis to face the camera
        baseTransform *= glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

        // Apply body part offsets and transforms relative to the base
        for (const auto &part : bodyParts) {
            glm::mat4 transform = baseTransform * glm::translate(glm::mat4(1.0f), part.offset) * part.transform;
            part.cube->render(shaderProgram, transform);
        }
    } else {
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        const auto &rightArm = bodyParts[2]; // Right arm at index 2

        glm::mat4 armTransform = glm::translate(glm::mat4(1.0f), _camera->getPosition());
        glm::vec3 forward = _camera->getFront();
        glm::vec3 up = _camera->getUp();
        glm::vec3 right = _camera->getRight();
        glm::mat4 viewRotation = glm::mat4(
            glm::vec4(right, 0.0f),
            glm::vec4(up, 0.0f),
            glm::vec4(-forward, 0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        armTransform = armTransform * viewRotation;

        armTransform = armTransform * glm::translate(glm::mat4(1.0f), armOffset);
        armTransform = armTransform * rightArm.transform;

        rightArm.cube->render(shaderProgram, armTransform);
        glEnable(GL_BLEND);
        if(DEPTH_TEST) {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LEQUAL);
        }
    }
    skinTexture->Unbind(1);
}
const char *Player::getState(void) const {
    if (dynamic_cast<WalkingState *>(currentState.get()))
        return "WALKING";
    if (dynamic_cast<RunningState *>(currentState.get()))
        return "RUNNING";
    if (dynamic_cast<SwimmingState *>(currentState.get()))
        return "SWIMMING";
    if (dynamic_cast<FlyingState *>(currentState.get()))
        return "FLYING";
    return "UNKNOWN";
}
const char *Player::getMode(void) const {
    if (dynamic_cast<SurvivalMode *>(currentMode.get()))
        return "SURVIVAL";
    if (dynamic_cast<CreativeMode *>(currentMode.get()))
        return "CREATIVE";
    if (dynamic_cast<SpectatorMode *>(currentMode.get()))
        return "SPECTATOR";
    return "UNKNOWN";
}
void Player::update(float deltaTime, ChunkManager &chunkManager) {
    input->update();
    animationTime += deltaTime;
    float speed = dynamic_cast<RunningState *>(currentState.get()) ? 6.0f : 4.0f;
    float amplitude = dynamic_cast<RunningState *>(currentState.get()) ? 0.7f : 0.5f;
    float swing = sin(animationTime * speed) * amplitude;

    if (isOnGround && (dynamic_cast<WalkingState *>(currentState.get()) || dynamic_cast<RunningState *>(currentState.get()))) {
        bodyParts[2].transform = glm::rotate(glm::mat4(1.0f), -swing, glm::vec3(1, 0, 0));
        bodyParts[3].transform = glm::rotate(glm::mat4(1.0f), swing, glm::vec3(1, 0, 0));
        bodyParts[4].transform = glm::rotate(glm::mat4(1.0f), swing, glm::vec3(1, 0, 0));
        bodyParts[5].transform = glm::rotate(glm::mat4(1.0f), -swing, glm::vec3(1, 0, 0));
    } else if (!isOnGround) {
        float bend = 0.3f;
        bodyParts[4].transform = glm::rotate(glm::mat4(1.0f), bend, glm::vec3(1, 0, 0));
        bodyParts[5].transform = glm::rotate(glm::mat4(1.0f), bend, glm::vec3(1, 0, 0));
        bodyParts[2].transform = glm::rotate(glm::mat4(1.0f), -bend / 2, glm::vec3(1, 0, 0));
        bodyParts[3].transform = glm::rotate(glm::mat4(1.0f), -bend / 2, glm::vec3(1, 0, 0));
    } else {
        for (auto &part : bodyParts)
            part.transform = glm::mat4(1.0f);
    }
    if (currentState)
        currentState->handleInput(*this, deltaTime); // Delegate movement to state

    // Apply gravity if not flying
    if (!isFlying && !isOnGround)
        velocity.y += GRAVITY * deltaTime;
    else if (isOnGround && velocity.y < 0.0f)
        velocity.y = 0.0f;

    glm::vec3 newPosition = position + pendingMovement + velocity * deltaTime;
    pendingMovement = glm::vec3(0.0f);
    modelMat = glm::translate(glm::mat4(1.0f), position);
    modelMat = glm::scale(modelMat, glm::vec3(scaleFactor));
    updateBoundingBox();

    // Handle collisions unless flying in Creative/Spectator
    if (dynamic_cast<SurvivalMode *>(currentMode.get()) || dynamic_cast<CreativeMode *>(currentMode.get()))
        handleCollisions(newPosition, velocity, position, chunkManager);

    if (newPosition != position) {
        position = newPosition;
        updateBoundingBox();
        updateCameraPosition();
    }
}

// Update camera position
void Player::updateCameraPosition(void) {
    if (isThirdPerson) {
        _camera->UpdateThirdPerson(position + glm::vec3(0.0f, eyeHeight, 0.0f));
    } else {
        _camera->setPosition(position + glm::vec3(0.0f, eyeHeight, 0.0f)); // Eye level
    }
}
// Handle jumping
void Player::jump(void) {
    if (isOnGround && !isFlying) {
        velocity.y = /*JUMP_FORCE*/ std::sqrt(2 * std::abs(GRAVITY) * h); // Maybe precalculate this at compile time or smth but please do not use sqrt()
        isOnGround = false;
    }
}
// ERROR: FIX THIS!!!!!!!
/*void Player::crouch(void) {
    if (!isCrouched) {
        prevPlayerHeight = playerHeight;
        playerHeight = 1.2f; // Reduced height for crouching (e.g., 1.2m)
        position.y -= (prevPlayerHeight - playerHeight); // Adjust position
        isCrouched = true;
    } else {
        // Check if there's space to uncrouch
        glm::vec3 testPos = position;
        testPos.y += (playerHeight - prevPlayerHeight);
        if (!handleCollisions(testPos, velocity, position, ChunkManager())) {
            playerHeight = prevPlayerHeight;
            position.y = testPos.y;
            isCrouched = false;
        }
    }
    updateCameraPosition();
}*/
// Get the player's current position
glm::vec3 Player::getPos(void) const {
    return position;
}

// Get the player's camera (for rendering or input handling)
Camera *Player::getCamera(void) const {
    return _camera;
}

// Set the player's position (and update the camera's position)
void Player::setPos(glm::vec3 newPos) {
    position = newPos;
    prevPosition = newPos; // Cache the new position to optimize the next update
    updateBoundingBox();
    updateCameraPosition();
}

// Handle mouse movement input
void Player::processMouseInput(ACTION action, ChunkManager &chunkManager) {
    if (action == PLACE_BLOCK && canPlaceBlocks)
        placeBlock(chunkManager);
    if (action == BREAK_BLOCK && canBreakBlocks)
        breakBlock(chunkManager);
}
void Player::breakBlock(ChunkManager &chunkManager) {
    // Perform a raycast to find the block the player is targeting
    std::optional<glm::ivec3> hitBlock = raycastVoxel(chunkManager, _camera->getPosition(), _camera->getFront(), max_interaction_distance);

    if (!hitBlock.has_value())
        return;

    glm::ivec3 blockPos = hitBlock.value();

    //log::system_info("Player", "Breaking block at: {}, {}, {}", blockPos.x, blockPos.y, blockPos.z);

    // Use updateBlock to set the block to AIR
    chunkManager.updateBlock(blockPos, Block::blocks::AIR);
}
void Player::placeBlock(ChunkManager &chunkManager) {
    // Perform raycast to find the block the player is looking at
    std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycastVoxelWithNormal(
        chunkManager, _camera->getPosition(), _camera->getFront(), max_interaction_distance);

    if (!hitResult.has_value())
        return; // No valid block hit, exit

    glm::ivec3 hitBlockPos = hitResult->first; // The block that was hit
    glm::ivec3 normal = hitResult->second;     // The normal of the hit face

    // Determine the  world position to place the new block
    glm::ivec3 placePos = hitBlockPos + (-normal);

    // Check if the placement position intersects with the player's bounding box
    if (isInsidePlayerBoundingBox(placePos))
        return;

    // Get the chunk where the new block should be placed
    std::shared_ptr<Chunk> placeChunk = chunkManager.getChunk(placePos);
    if (!placeChunk) {
        return;
    }

    // Convert world coordinates to local chunk coordinates
    glm::ivec3 localBlockPos = Chunk::worldToLocal(placePos, chunkSize);

    // Validate block index
    int blockIndex = placeChunk->getBlockIndex(localBlockPos.x, localBlockPos.y, localBlockPos.z);
    if (blockIndex < 0 || static_cast<size_t>(blockIndex) >= placeChunk->getChunkData().size()) {
        log::system_info("Player", "❌ Invalid block index: {}", blockIndex);
        return;
    }

    // Check if the target position is air
    if (placeChunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
        log::system_info("Player", "❌ Target block is NOT air! It's type: {}", Block::toString(placeChunk->getChunkData()[blockIndex].type));
        return;
    }

    //log::system_info("Player", "Placing block at: {}, {}, {}", placePos.x, placePos.y, placePos.z);

    // Place the block using updateBlock
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
        // Get the chunk the voxel belongs to
        std::shared_ptr<Chunk> chunk = chunkManager.getChunk({worldResult.x, 0, worldResult.z});

        if (chunk) {
            glm::ivec3 localVoxelPos = Chunk::worldToLocal(worldResult, chunkSize);
            int blockIndex = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

            if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
                chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
                return worldResult; // Found a solid block
            }
        }

        // Move to the next voxel along the ray
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

    return std::nullopt; // No solid block found within range
}

std::optional<std::pair<glm::ivec3, glm::ivec3>> Player::raycastVoxelWithNormal(
    ChunkManager &chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance) {
    glm::ivec3 worldResult = glm::floor(rayOrigin);  // Start voxel
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
        // ** Get the chunk that contains this voxel **
       Chunk *chunk = chunkManager.getChunk({worldResult.x, 0, worldResult.z}, true);

        if (chunk) {
            glm::ivec3 localVoxelPos = Chunk::worldToLocal(worldResult, chunkSize);
            int blockIndex = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

            if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
                chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
                return std::make_pair(worldResult, worldResult - lastVoxel);
            }
        }

        lastVoxel = worldResult;

        // ** Move to next voxel **
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

// Handle mouse movement input
void Player::processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    _camera->ProcessMouseMovement(xoffset, yoffset, constrainPitch);
}
// Handle mouse scroll input
void Player::processMouseScroll(float yoffset) {
    // Adjust movement speed in first-person
    float scroll_speed_multiplier = 1.0f;
    if (dynamic_cast<FlyingState *>(currentState.get()) && dynamic_cast<SpectatorMode *>(currentMode.get())) {
        flying_speed += yoffset;
        if (flying_speed <= 0)
            flying_speed = 0;
    } else if (!isThirdPerson) {
        selectedBlock += (int)(yoffset * scroll_speed_multiplier);
        if (selectedBlock < 1)
            selectedBlock = 1;
        if (selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS))
            selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
    }
    _camera->ProcessMouseScroll(yoffset);
}
