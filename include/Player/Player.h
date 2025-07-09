#pragma once
#include <memory>
#include <optional>
#include <GL/glew.h>
#include "defines.h"
#include "../InputManager.h"
#include "ChunkManager.h"
#include "Camera.h"
#include "Cube.h"
#include "AABB.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

class PlayerState;
class PlayerMode;

class Player {
  public:
    Player(glm::vec3 spawnPos, std::shared_ptr<InputManager> input);
    ~Player();

    enum ACTION { BREAK_BLOCK,
                  PLACE_BLOCK };

    glm::vec3 getPos(void) const;
    Camera *getCamera(void) const;
    const char *getMode(void) const;  // For debugging
    const char *getState(void) const; // For debugging

    template <typename Mode, typename... Args>
        void changeMode(Args&&... args) {
            static_assert(std::is_base_of<PlayerMode, Mode>::value, "Mode must derive from PlayerMode");
            changeMode(std::make_unique<Mode>(std::forward<Args>(args)...));
        }

    template <typename State, typename... Args>
        void changeState(Args&&... args) {
            static_assert(std::is_base_of<PlayerState, State>::value, "Mode must derive from PlayerMode");
            changeState(std::make_unique<State>(std::forward<Args>(args)...));
        }


    void changeMode(std::unique_ptr<PlayerMode> newMode);
    void changeState(std::unique_ptr<PlayerState> newState);
    void update(float deltaTime, ChunkManager &chunkManager);
    void render(const Shader &shader);
    void loadSkin(const std::string &path);
    void loadSkin(const std::filesystem::path &path);
    void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch);
    void processMouseInput(ChunkManager &chunkManager);
    void processKeyInput(bool FREE_CURSOR);
    void processMouseScroll(float yoffset);
    void setPos(glm::vec3 newPos);
    void toggleCameraMode(void);
    void jump(void);
    void crouch(void);

    glm::mat4 modelMat;

    const glm::mat4 &getModelMatrix(void) const {
        return modelMat;
    }

    float ExtentX = 0.4f;
    float ExtentY = 0.4f;

    AABB aabb;
    const AABB &getAABB(void) const {
        return aabb;
    }
    void updateBoundingBox(void) {
        updatePlayerBoundingBox(position);
    }
    void updatePlayerBoundingBox(const glm::vec3 &pos) {
        glm::vec3 min = pos - glm::vec3(ExtentX, 0.0f, ExtentY);
        glm::vec3 max = pos + glm::vec3(ExtentX, playerHeight, ExtentY);
        aabb = AABB(min, max);
    }
    AABB getBoundingBoxAt(const glm::vec3 &pos) const {
        glm::vec3 min = pos - glm::vec3(ExtentX, 0.0f, ExtentY);
        glm::vec3 max = pos + glm::vec3(ExtentX, playerHeight, ExtentY);
        return AABB(min, max);
    }

    std::optional<std::pair<glm::ivec3, glm::ivec3>> raycastVoxelWithNormal(ChunkManager &chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance);
    std::optional<glm::ivec3> raycastVoxel(ChunkManager &chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance);
    void handleCollisions(glm::vec3 &newPosition, glm::vec3 &velocity, const glm::vec3 &oldPosition, ChunkManager &chunkManager);
    bool isCollidingAt(const glm::vec3 &pos, ChunkManager &chunkManager);
    bool isInsidePlayerBoundingBox(const glm::vec3 &checkPos) const;
    void breakBlock(ChunkManager &chunkManager);
    void placeBlock(ChunkManager &chunkManager);

    // Public members
    std::unique_ptr<PlayerState> currentState;
    std::unique_ptr<PlayerMode> currentMode;
    std::shared_ptr<InputManager> input;
    std::unique_ptr<Texture> skinTexture;
    Camera *_camera;

    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 velocity;
    glm::vec3 pendingMovement = glm::vec3(0.0f);

    // -- Player Model ---
    struct BodyPart {
        std::unique_ptr<Cube> cube;
        glm::vec3 offset;
        glm::mat4 transform = glm::mat4(1.0f);
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
    

    GLuint skinVAO;


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
    const float h = 1.252f; // Height of the jump of the player
    float max_interaction_distance = 10.0f;
    unsigned int render_distance = 5;

    float playerHeight = 1.8f;
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
