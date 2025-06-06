#pragma once
#include <memory>
#include <optional>
#include <GL/glew.h>
#include "defines.h"
#include "../InputManager.h"
#include "ChunkManager.h"
#include "Cube.h"

class PlayerState;
class PlayerMode;

class Player {
public:
    Player(glm::vec3 spawnPos, glm::ivec3& chunksize, GLFWwindow* window);
    ~Player(void);

    enum ACTION { BREAK_BLOCK, PLACE_BLOCK };


    glm::vec3 getPos(void) const;
    Camera* getCamera(void) const;
    const char* getMode(void) const;  // For debugging
    const char* getState(void) const; // For debugging

    
    void changeMode(std::unique_ptr<PlayerMode> newMode);
    void changeState(std::unique_ptr<PlayerState> newState);
    void update(float deltaTime, ChunkManager& chunkManager);
    void update(float deltaTime);
    void render(unsigned int shaderProgram);
    void loadSkin(const std::string &path);
    void loadSkin(const std::filesystem::path &path);
    void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch);
    void processMouseInput(ACTION action, ChunkManager& chunkManager);
    void processMouseScroll(float yoffset);
    void setPos(glm::vec3 newPos);
    void toggleCameraMode(void);
    void jump(void);
    void crouch(void);


    float halfWidth = 0.4f;
    float halfDepth = 0.4f;
    float halfHeight;


    std::optional<glm::ivec3> raycastVoxel(ChunkManager& chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance);
    std::optional<std::pair<glm::ivec3, glm::ivec3>> raycastVoxelWithNormal(ChunkManager& chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance);
    void handleCollisions(glm::vec3& newPosition, glm::vec3& velocity, const glm::vec3& oldPosition, ChunkManager& chunkManager);
    bool isCollidingAt(const glm::vec3& pos, ChunkManager& chunkManager);
    bool isInsidePlayerBoundingBox(const glm::vec3& checkPos) const;
    void breakBlock(ChunkManager& chunkManager);
    void placeBlock(ChunkManager& chunkManager);


    // Public members
    std::unique_ptr<PlayerState> currentState;
    std::unique_ptr<PlayerMode> currentMode;
    std::unique_ptr<InputManager> input;
    std::unique_ptr<Texture> skinTexture;
    Camera *_camera;

    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 velocity;
    glm::vec3 chunkSize;
    glm::vec3 pendingMovement = glm::vec3(0.0f);


    // -- Player Model ---
    struct BodyPart {
        std::unique_ptr<Cube> cube;
        glm::vec3 offset;
        glm::mat4 transform;
    };
    std::vector<BodyPart> bodyParts;
    
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

    glm::vec3 armOffset = glm::vec3(0.3f, -0.792f, -0.5f); // Right, down, forward
    // --- Player Settings ---
    float animationTime;
    float scaleFactor;
    float lastScaleFactor;
    float walking_speed = 3.6f;
    float running_speed_increment = 4.2f;
    float running_speed = walking_speed + running_speed_increment;
    float swimming_speed = 8.0f;
    float flying_speed = 17.5f;
    float JUMP_FORCE = 4.5f;
    const float h = 1.252f;	// Height of the jump of the player
    float max_interaction_distance = 10.0f;
    unsigned int render_distance = 5;

    float playerHeight = 0.0f;
    float prevPlayerHeight;
    float eyeHeight;
    int selectedBlock = 1;



    // --- Player Flags ---
    bool isOnGround = false;
    bool isDamageable = false;
    bool isRunning = false;
    bool isFlying = false;
    bool isSwimming = false;
    bool isWalking = false;
    bool isThirdPerson = false;
    bool canPlaceBlocks = false;
    bool canBreakBlocks = false;
    bool renderSkin = false;
    // TODO: Implement these ones
    bool canFly = true;
    bool canWalk = true;
    bool canRun = true;
    bool isCrouched = false;
    bool hasHealth = true;
    bool hasInfiniteBlocks = false;
private:
    void updateCameraPosition(void);
    void setupBodyParts(void);
};

