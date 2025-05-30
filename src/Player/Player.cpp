#include "Player.h"
#include "PlayerState.h"  // Full definition of PlayerState
#include "PlayerMode.h"   // Full definition of PlayerMode
#include "FlyingState.h"
#include "WalkingState.h"
#include "SurvivalMode.h"
#include "CreativeMode.h"
#include "SpectatorMode.h"
#include "defines.h"
#include <memory>
#include <numbers>

// Constructor
Player::Player(glm::vec3 spawnPos, glm::ivec3& chunksize, GLFWwindow* window)
    : _camera(new Camera(spawnPos)), position(spawnPos), prevPosition(spawnPos), velocity(0.0f), chunkSize(chunksize), animationTime(0.0f), scaleFactor(0.076f), playerHeight(1.8f), prevPlayerHeight(playerHeight) {
    halfHeight = playerHeight / 2.0f;
    eyeHeight = playerHeight * 0.9f;
    lastScaleFactor = scaleFactor;
    input = std::make_unique<InputManager>(window);
    skinTexture = std::make_unique<Texture>(DEFAULT_SKIN_DIRECTORY, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);  // Default skin
    setupBodyParts();
    currentMode = std::make_unique<SurvivalMode>();	// STARTING MODE DEFAULT
    currentMode->enterMode(*this); // Sets initial state via mode
    updateCameraPosition();
}
// Toggle between first-person and third-person
void Player::toggleCameraMode(void) {
    if (isThirdPerson) {
        // Switch to first-person
        _camera->SwitchToFirstPerson(position + glm::vec3(0.0f, eyeHeight, 0.0f));
    } else {
        // Switch to third-person, preserving the current first-person orientation
        _camera->SwitchToThirdPerson(position + glm::vec3(0.0f, eyeHeight, 0.0f), 
                                    5.0f,  // Default distance
                                    _camera->Yaw,  // Use current Yaw from first-person
                                    _camera->Pitch);  // Use current Pitch from first-person
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

    bodyParts[0] = { std::make_unique<Cube>(headSize,  BodyPartType::HEAD),  headOffset,     glm::mat4(1.0f) };
    bodyParts[1] = { std::make_unique<Cube>(torsoSize, BodyPartType::TORSO), torsoOffset,    glm::mat4(1.0f) };
    bodyParts[2] = { std::make_unique<Cube>(limbSize,  BodyPartType::RIGHT_ARM), rightArmOffset, glm::mat4(1.0f) };
    bodyParts[3] = { std::make_unique<Cube>(limbSize,  BodyPartType::LEFT_ARM), leftArmOffset,  glm::mat4(1.0f) };
    bodyParts[4] = { std::make_unique<Cube>(limbSize,  BodyPartType::RIGHT_LEG), rightLegOffset, glm::mat4(1.0f) };
    bodyParts[5] = { std::make_unique<Cube>(limbSize,  BodyPartType::LEFT_LEG), leftLegOffset,  glm::mat4(1.0f) };

}
void Player::loadSkin(const std::string &path) {
    skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
void Player::loadSkin(const std::filesystem::path &path) {
    skinTexture = std::make_unique<Texture>(path, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST);
}
void Player::changeMode(std::unique_ptr<PlayerMode> newMode) {
    if (currentMode)
	currentMode->exitMode(*this); // Clean up previous mode if necessary

    currentMode = std::move(newMode); // Ownership transferred to currentMode

    if (currentMode)
	currentMode->enterMode(*this);
}

// Change the state by moving a new state into currentState
void Player::changeState(std::unique_ptr<PlayerState> newState) {
    if (currentState)
	currentState->exitState(*this); // Clean up previous state if necessary

    currentState = std::move(newState); // Ownership transferred to currentState

    if (currentState)
	currentState->enterState(*this);
}

// Destructor
Player::~Player(void)
{
    delete _camera;
}
std::optional<glm::ivec3> Player::raycastVoxel(ChunkManager& chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance) 
{
    // Initialize voxel position (world-space)
    glm::ivec3 voxel(
        static_cast<int>(std::floor(rayOrigin.x)),
        static_cast<int>(std::floor(rayOrigin.y)),
        static_cast<int>(std::floor(rayOrigin.z))
    );

    // Step direction for each axis
    glm::ivec3 step(
        (rayDirection.x >= 0) ? 1 : -1,
        (rayDirection.y >= 0) ? 1 : -1,
        (rayDirection.z >= 0) ? 1 : -1
    );

    // Compute tMax: distance until first voxel boundary is crossed
    glm::vec3 tMax(
        (rayDirection.x != 0.0f) ? ((step.x > 0 ? (voxel.x + 1.0f - rayOrigin.x) : (rayOrigin.x - voxel.x)) / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
        (rayDirection.y != 0.0f) ? ((step.y > 0 ? (voxel.y + 1.0f - rayOrigin.y) : (rayOrigin.y - voxel.y)) / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
        (rayDirection.z != 0.0f) ? ((step.z > 0 ? (voxel.z + 1.0f - rayOrigin.z) : (rayOrigin.z - voxel.z)) / std::abs(rayDirection.z)) : std::numeric_limits<float>::max()
    );

    // Compute tDelta: how far to move in t to cross a voxel
    glm::vec3 tDelta(
        (rayDirection.x != 0.0f) ? (1.0f / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
        (rayDirection.y != 0.0f) ? (1.0f / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
        (rayDirection.z != 0.0f) ? (1.0f / std::abs(rayDirection.z)) : std::numeric_limits<float>::max()
    );

    float t = 0.0f;
    while (t <= maxDistance) 
    {
        // Get the chunk the voxel belongs to
        Chunk* chunk = chunkManager.getChunk({voxel.x, 0, voxel.z}, true);
        
        if (chunk) 
        {
            glm::ivec3 localVoxelPos = Chunk::worldToLocal(voxel, chunkSize);
            int blockIndex = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

            if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
                chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) 
            {
                return voxel;  // Found a solid block
            }
        }

        // Move to the next voxel along the ray
        if (tMax.x < tMax.y) 
        {
            if (tMax.x < tMax.z) 
            {
                voxel.x += step.x;
                t = tMax.x;
                tMax.x += tDelta.x;
            } 
            else 
            {
                voxel.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        } 
        else 
        {
            if (tMax.y < tMax.z) 
            {
                voxel.y += step.y;
                t = tMax.y;
                tMax.y += tDelta.y;
            } 
            else 
            {
                voxel.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        }
    }

    return std::nullopt; // No solid block found within range
}
bool Player::isCollidingAt(const glm::vec3& pos, ChunkManager& chunkManager) {
    int minX = static_cast<int>(std::floor(pos.x - halfWidth));
    int maxX = static_cast<int>(std::floor(pos.x + halfWidth));
    int minY = static_cast<int>(std::floor(pos.y));
    int maxY = static_cast<int>(std::floor(pos.y + playerHeight));
    int minZ = static_cast<int>(std::floor(pos.z - halfDepth));
    int maxZ = static_cast<int>(std::floor(pos.z + halfDepth));

    for (int x = minX; x <= maxX; ++x) {
        for (int y = minY; y <= maxY; ++y) {
            for (int z = minZ; z <= maxZ; ++z) {
                glm::ivec3 chunkCoords = Chunk::worldToChunk(glm::vec3(x, y, z), chunkSize);
                const auto& currentChunk = chunkManager.getChunk({x, 0, z});
                if (!currentChunk) return true; // Prevent moving into unloaded chunks

                int localX = x - (chunkCoords.x * chunkSize.x);
		int localY = y - (chunkCoords.y * chunkSize.y);
                int localZ = z - (chunkCoords.z * chunkSize.z);

                if (y < 0 || y >= chunkSize.y) {
                    if (y < 0) return true;
                    else continue;
                }

                const Block& block = currentChunk->getBlockAt(localX, localY, localZ);
                if (block.type != Block::blocks::AIR) {
                    return true;
                }
            }
        }
    }
    return false;
}
bool Player::isInsidePlayerBoundingBox(const glm::vec3& checkPos) const {
    int minX = static_cast<int>(std::floor(position.x - halfWidth));
    int maxX = static_cast<int>(std::floor(position.x + halfWidth));
    int minY = static_cast<int>(std::floor(position.y));
    int maxY = static_cast<int>(std::floor(position.y + playerHeight));
    int minZ = static_cast<int>(std::floor(position.z - halfDepth));
    int maxZ = static_cast<int>(std::floor(position.z + halfDepth));

    int checkX = static_cast<int>(std::floor(checkPos.x));
    int checkY = static_cast<int>(std::floor(checkPos.y));
    int checkZ = static_cast<int>(std::floor(checkPos.z));

    bool inside = (checkX >= minX && checkX <= maxX) &&
	(checkY >= minY && checkY <= maxY) &&
	(checkZ >= minZ && checkZ <= maxZ);
    return inside;
}
/*
void Player::handleCollisions(glm::vec3& newPosition, glm::vec3& velocity,
                              const glm::vec3& oldPosition, ChunkManager& chunkManager) {
    // --- Y-axis collision (handle vertical first to set correct y-position) ---
    glm::vec3 testPos = newPosition;
    if (isCollidingAt(testPos, chunkManager)) {
        if (velocity.y < 0)
            isOnGround = true;
        newPosition.y = oldPosition.y;
        velocity.y = 0.0f;
    } else {
        isOnGround = false;
    }

    // --- X-axis collision ---
    testPos = newPosition; // Use newPosition.y (after Y-axis collision)
    testPos.z = oldPosition.z; // Only change z to test x movement
    if (isCollidingAt(testPos, chunkManager)) {
        newPosition.x = oldPosition.x;
        velocity.x = 0.0f;
    }

    // --- Z-axis collision ---
    testPos = newPosition; // Use newPosition.y (after Y-axis collision)
    testPos.x = oldPosition.x; // Only change x to test z movement
    if (isCollidingAt(testPos, chunkManager)) {
        newPosition.z = oldPosition.z;
        velocity.z = 0.0f;
    }

    // Additional check for grounded state when velocity.y is zero
    if (velocity.y == 0.0f) {
        glm::vec3 groundCheckPos = newPosition;
        groundCheckPos.y -= 0.001f; // Small epsilon to check for ground
        if (isCollidingAt(groundCheckPos, chunkManager)) {
            isOnGround = true;
        }
    }
}*/
void Player::handleCollisions(glm::vec3& newPosition, glm::vec3& velocity,
                              const glm::vec3& oldPosition, ChunkManager& chunkManager) {
    // --- Full movement collision check ---
    glm::vec3 testPos = newPosition;
    if (isCollidingAt(testPos, chunkManager)) {
        // Try Y-axis first (vertical collision)
        testPos = newPosition;
        testPos.x = oldPosition.x;
        testPos.z = oldPosition.z;
        if (isCollidingAt(testPos, chunkManager)) {
            if (velocity.y < 0)
                isOnGround = true;
            newPosition.y = oldPosition.y;
            velocity.y = 0.0f;
        } else {
            isOnGround = false;
        }

        // Test X-axis (horizontal collision)
        testPos = newPosition;
        testPos.z = oldPosition.z;
        if (isCollidingAt(testPos, chunkManager)) {
            newPosition.x = oldPosition.x;
            velocity.x = 0.0f;
        }

        // Test Z-axis (horizontal collision)
        testPos = newPosition;
        testPos.x = oldPosition.x;
        if (isCollidingAt(testPos, chunkManager)) {
            newPosition.z = oldPosition.z;
            velocity.z = 0.0f;
        }
    } else {
        isOnGround = false;
    }

    // Ground check for standing still
    if (velocity.y == 0.0f) {
        glm::vec3 groundCheckPos = newPosition;
        groundCheckPos.y -= 0.001f;
        if (isCollidingAt(groundCheckPos, chunkManager)) {
            isOnGround = true;
        }
    }
}
void Player::render(unsigned int shaderProgram) {
    if (!skinTexture) {
	std::cerr << "Error: skinTexture is null!" << std::endl;
	return;
    }

    skinTexture->Bind(2);

    if(isThirdPerson)
    {
	// Calculate the total height of the player model (torso + head, scaled)
	//float modelHeight = (torsoSize.y + headSize.y) * scaleFactor;  // Total height in Minecraft units, scaled

	// Start with the player's position (feet at position.y)
	glm::mat4 baseTransform = glm::translate(glm::mat4(1.0f), position);

	// Rotate the player to face the camera's Front direction
	glm::vec3 cameraDirection = _camera->Front;
	// Flatten the camera direction to ignore Y (vertical) component for horizontal facing
	cameraDirection.y = 0.0f;
	cameraDirection = glm::normalize(cameraDirection);

	// Player's forward direction (initially facing -Z, or (0, 0, -1))
	glm::vec3 playerForward(0.0f, 0.0f, -1.0f);

	// Calculate the rotation angle around the Y-axis
	float dot = glm::dot(playerForward, cameraDirection);
	dot = glm::clamp(dot, -1.0f, 1.0f); // Ensure dot product is within valid range
	float angle = acos(dot) * (180.0f / std::numbers::pi); // Convert to degrees

	// Determine rotation direction (cross product for right-hand rule)
	glm::vec3 cross = glm::cross(playerForward, cameraDirection);
	if (cross.y < 0) angle = -angle; // Rotate clockwise if cross.y is negative

	// Apply rotation around Y-axis to face the camera
	baseTransform = baseTransform * glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));

	// Apply body part offsets and transforms relative to the base
	for (const auto& part : bodyParts) {
	    glm::mat4 transform = baseTransform * glm::translate(glm::mat4(1.0f), part.offset) * part.transform;
	    part.cube->render(shaderProgram, transform);
	}
    }
    else{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	const auto& rightArm = bodyParts[2]; // Right arm at index 2

	glm::mat4 armTransform = glm::translate(glm::mat4(1.0f), _camera->Position);
	glm::vec3 forward = _camera->Front;
	glm::vec3 up = _camera->Up;
	glm::vec3 right = _camera->Right;
	glm::mat4 viewRotation = glm::mat4(
		glm::vec4(right, 0.0f),
		glm::vec4(up, 0.0f),
		glm::vec4(-forward, 0.0f),
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
		);
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
    skinTexture->Unbind();
}
const char* Player::getState(void) const {
    if (dynamic_cast<WalkingState*>(currentState.get())) return "WALKING";
    if (dynamic_cast<RunningState*>(currentState.get())) return "RUNNING";
    if (dynamic_cast<SwimmingState*>(currentState.get())) return "SWIMMING";
    if (dynamic_cast<FlyingState*>(currentState.get())) return "FLYING";
    return "UNKNOWN";
}
const char* Player::getMode(void) const {
    if (dynamic_cast<SurvivalMode*>(currentMode.get())) return "SURVIVAL";
    if (dynamic_cast<CreativeMode*>(currentMode.get())) return "CREATIVE";
    if (dynamic_cast<SpectatorMode*>(currentMode.get())) return "SPECTATOR";
    return "UNKNOWN";
}
void Player::update(float deltaTime, ChunkManager& chunkManager) {
    input->update();
    animationTime += deltaTime;
    float speed = dynamic_cast<RunningState*>(currentState.get()) ? 6.0f : 4.0f;
    float amplitude = dynamic_cast<RunningState*>(currentState.get()) ? 0.7f : 0.5f;
    float swing = sin(animationTime * speed) * amplitude;

    if (isOnGround && (dynamic_cast<WalkingState*>(currentState.get()) || dynamic_cast<RunningState*>(currentState.get()))) {
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
        for (auto& part : bodyParts) part.transform = glm::mat4(1.0f);
    }
    if (currentState)
        currentState->handleInput(*this, deltaTime); // Delegate movement to state

    // Apply gravity if not flying
    if (!isFlying && !isOnGround)
        velocity.y -= GRAVITY * deltaTime;
    else if (isOnGround && velocity.y < 0.0f)
        velocity.y = 0.0f;

    glm::vec3 newPosition = position + pendingMovement + velocity * deltaTime;
    pendingMovement = glm::vec3(0.0f);

    // Handle collisions unless flying in Creative/Spectator
    if (dynamic_cast<SurvivalMode*>(currentMode.get()) || dynamic_cast<CreativeMode*>(currentMode.get()))
	handleCollisions(newPosition, velocity, position, chunkManager);

    if (newPosition != position) {
        position = newPosition;
        updateCameraPosition();
    }

}

void Player::update(float deltaTime) {
    input->update();
    animationTime += deltaTime;
    float speed = dynamic_cast<RunningState*>(currentState.get()) ? 6.0f : 4.0f;
    float amplitude = dynamic_cast<RunningState*>(currentState.get()) ? 0.7f : 0.5f;
    float swing = sin(animationTime * speed) * amplitude;

    if (isOnGround && (dynamic_cast<WalkingState*>(currentState.get()) || dynamic_cast<RunningState*>(currentState.get()))) {
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
        for (auto& part : bodyParts) part.transform = glm::mat4(1.0f);
    }
    if (currentState)
        currentState->handleInput(*this, deltaTime); // Delegate movement to state

    // Apply gravity if not flying
    if (!isFlying && !isOnGround)
        velocity.y -= GRAVITY * deltaTime;
    else if (isOnGround && velocity.y < 0.0f)
        velocity.y = 0.0f;

    glm::vec3 newPosition = position + pendingMovement + velocity * deltaTime;
    pendingMovement = glm::vec3(0.0f);


    if (newPosition != position) {
        position = newPosition;
        updateCameraPosition();
    }
}


// Update camera position
void Player::updateCameraPosition(void) {
    if (isThirdPerson) {
        _camera->UpdateThirdPerson(position + glm::vec3(0.0f, eyeHeight, 0.0f));
    } else {
        _camera->Position = position + glm::vec3(0.0f, eyeHeight, 0.0f); // Eye level
        _camera->updateCameraVectors();
    }
}
// Handle jumping
void Player::jump(void) {
    if (isOnGround && !isFlying) {
        velocity.y = JUMP_FORCE;
        isOnGround = false;
    }
}
//ERROR: FIX THIS!!!!!!!
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
glm::vec3 Player::getPos(void) const
{
    return position;
}

// Get the player's camera (for rendering or input handling)
Camera* Player::getCamera(void) const
{
    return _camera;
}

// Set the player's position (and update the camera's position)
void Player::setPos(glm::vec3 newPos)
{
    position = newPos;
    prevPosition = newPos;  // Cache the new position to optimize the next update
    updateCameraPosition();
}

// Handle mouse movement input
void Player::processMouseInput(ACTION action, ChunkManager& chunkManager)
{
    if(action == PLACE_BLOCK && canPlaceBlocks)
	placeBlock(chunkManager);
    if(action == BREAK_BLOCK && canBreakBlocks)
	breakBlock(chunkManager);
}
void Player::breakBlock(ChunkManager& chunkManager) {
    // Perform a raycast to find the block the player is targeting
    std::optional<glm::ivec3> hitBlock = raycastVoxel(chunkManager, _camera->Position, _camera->Front, max_interaction_distance);

    if (!hitBlock.has_value()) return;

    glm::ivec3 blockPos = hitBlock.value();

    // Use updateBlock to set the block to AIR
    chunkManager.updateBlock(blockPos, Block::blocks::AIR);
}
std::optional<std::pair<glm::ivec3, glm::ivec3>> Player::raycastVoxelWithNormal(
    ChunkManager& chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance)
{
    glm::ivec3 voxel = glm::floor(rayOrigin); // Start voxel
    glm::ivec3 step = glm::sign(rayDirection); // Step direction (-1 or +1)

    glm::vec3 tMax;
    glm::vec3 tDelta = glm::vec3(
        (rayDirection.x != 0.0f) ? (1.0f / std::abs(rayDirection.x)) : std::numeric_limits<float>::max(),
        (rayDirection.y != 0.0f) ? (1.0f / std::abs(rayDirection.y)) : std::numeric_limits<float>::max(),
        (rayDirection.z != 0.0f) ? (1.0f / std::abs(rayDirection.z)) : std::numeric_limits<float>::max()
    );

    for (int i = 0; i < 3; i++) {
        if (rayDirection[i] > 0)
            tMax[i] = (voxel[i] + 1 - rayOrigin[i]) * tDelta[i];
        else
            tMax[i] = (rayOrigin[i] - voxel[i]) * tDelta[i];
    }

    float t = 0.0f;
    glm::ivec3 lastVoxel = voxel;

    while (t < maxDistance) {
        // ** Get the chunk that contains this voxel **
        Chunk* chunk = chunkManager.getChunk({voxel.x, 0, voxel.z}, true);
        
        if (chunk) {
            glm::ivec3 localVoxelPos = Chunk::worldToLocal(voxel, chunkSize);
            int blockIndex = chunk->getBlockIndex(localVoxelPos.x, localVoxelPos.y, localVoxelPos.z);

            if (blockIndex >= 0 && static_cast<size_t>(blockIndex) < chunk->getChunkData().size() &&
                chunk->getChunkData()[blockIndex].type != Block::blocks::AIR) 
            {
                return std::make_pair(voxel, voxel - lastVoxel);
            }
        }

        lastVoxel = voxel;

        // ** Move to next voxel **
        if (tMax.x < tMax.y) {
            if (tMax.x < tMax.z) {
                voxel.x += step.x;
                t = tMax.x;
                tMax.x += tDelta.x;
            } else {
                voxel.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        } else {
            if (tMax.y < tMax.z) {
                voxel.y += step.y;
                t = tMax.y;
                tMax.y += tDelta.y;
            } else {
                voxel.z += step.z;
                t = tMax.z;
                tMax.z += tDelta.z;
            }
        }
    }

    return std::nullopt;
}

void Player::placeBlock(ChunkManager& chunkManager) {
    // Perform raycast to find the block the player is looking at
    std::optional<std::pair<glm::ivec3, glm::ivec3>> hitResult = raycastVoxelWithNormal(
        chunkManager, _camera->Position, _camera->Front, max_interaction_distance);

    if (!hitResult.has_value()) return; // No valid block hit, exit

    glm::ivec3 hitBlockPos = hitResult->first;  // The block that was hit
    glm::ivec3 normal = hitResult->second;      // The normal of the hit face

    // Determine the position to place the new block
    glm::ivec3 placePos = hitBlockPos + (-normal);

    // Check if the placement position intersects with the player's bounding box
    if (isInsidePlayerBoundingBox(placePos)) {
        return;
    }

    // Get the chunk where the new block should be placed
    Chunk* placeChunk = chunkManager.getChunk(placePos, true);
    if (!placeChunk) {
        return;
    }

    // Convert world coordinates to local chunk coordinates
    glm::ivec3 localBlockPos = Chunk::worldToLocal(placePos, chunkSize);

    // Validate block index
    int blockIndex = placeChunk->getBlockIndex(localBlockPos.x, localBlockPos.y, localBlockPos.z);
    if (blockIndex < 0 || static_cast<size_t>(blockIndex) >= placeChunk->getChunkData().size()) {
        std::cout << "❌ Invalid block index: " << blockIndex << "\n";
        return;
    }

    // Check if the target position is air
    if (placeChunk->getChunkData()[blockIndex].type != Block::blocks::AIR) {
        std::cout << "❌ Target block is NOT air! It's type: "
                  << Block::toString(placeChunk->getChunkData()[blockIndex].type) << "\n";
        return;
    }

    // Place the block using updateBlock
    chunkManager.updateBlock(placePos, static_cast<Block::blocks>(selectedBlock));
}

// Handle mouse movement input
void Player::processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    _camera->ProcessMouseMovement(xoffset, yoffset, constrainPitch);
}
// Handle mouse scroll input
void Player::processMouseScroll(float yoffset)
{
    // Adjust movement speed in first-person
    float scroll_speed_multiplier = 1.0f;
    if(dynamic_cast<FlyingState*>(currentState.get()) && dynamic_cast<SpectatorMode*>(currentMode.get()))
    {
	flying_speed += yoffset;
	if(flying_speed <= 0)	flying_speed = 0;
    }
    else if(!isThirdPerson)
    {
	selectedBlock += (int)(yoffset * scroll_speed_multiplier);
	if (selectedBlock < 1) selectedBlock = 1;
	if (selectedBlock >= Block::toInt(Block::blocks::MAX_BLOCKS)) selectedBlock = Block::toInt(Block::blocks::MAX_BLOCKS) - 1;
    }
    _camera->ProcessMouseScroll(yoffset);
}
