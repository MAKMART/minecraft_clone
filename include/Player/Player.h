#pragma once
#include <memory>
#include <optional>
#include <GL/glew.h>
#include "core/defines.h"
#include "core/InputManager.hpp"
#include "chunk/ChunkManager.h"
#include "core/CameraController.hpp"
#include "Cube.h"
#include "core/AABB.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

class PlayerState;
class PlayerMode;

class Player {
  public:

    Player(glm::vec3 spawnPos, std::shared_ptr<InputManager> _input);
    ~Player();

    enum ACTION { BREAK_BLOCK, PLACE_BLOCK };

    const glm::vec3 &getPos() const { return position; }
    // Non-const &: allows modification
    CameraController& getCameraController() { return camCtrl; }
    // Const &: allows read-only access
    const CameraController& getCameraController() const { return camCtrl; }
    const Camera &getCamera() const { return camCtrl.getCamera(); }

    glm::mat4 getViewMatrix() const { return camCtrl.getViewMatrix(); }
    glm::mat4 getProjectionMatrix() const { return camCtrl.getProjectionMatrix(); };
    glm::vec3 getCameraFront() const { return camCtrl.getFront(); };
    glm::vec3 getCameraRight() const { return camCtrl.getRight(); };
    glm::vec3 getCameraUp() const { return camCtrl.getUp(); };
    const glm::mat4 &getModelMatrix() const { return modelMat; }

    // Non-const ref can be changed
    glm::vec3 &getArmOffset() { return armOffset; };


    bool isCameraThirdPerson() const { return camCtrl.isThirdPersonMode(); }

    const char *getMode() const;  // For debugging
    const char *getState() const; // For debugging



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


    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch);
    void processMouseInput(ChunkManager &chunkManager);
    void processKeyInput();
    void processMouseScroll(float yoffset);


    void setPos(glm::vec3 newPos);

    void jump();

    void crouch();


    const AABB &getAABB() const {
        return aabb;
    }
    void updateBoundingBox() {
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

    bool isInsidePlayerBoundingBox(const glm::vec3 &checkPos) const;
    void breakBlock(ChunkManager &chunkManager);
    void placeBlock(ChunkManager &chunkManager);

    // Public members
    std::unique_ptr<PlayerState> currentState;
    std::unique_ptr<PlayerMode> currentMode;
    std::shared_ptr<InputManager> input;




    void toggleCameraMode() {
        camCtrl.toggleThirdPersonMode();
    }

    void updateCamPos() {
        if (camCtrl.isThirdPersonMode())
            camCtrl.setOrbitTarget(position + glm::vec3(0, eyeHeight, 0));
        else
            camCtrl.setPosition(position + glm::vec3(0, eyeHeight, 0));
    }
    // --- Player Settings ---
    float animationTime = 0.0f;
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

    float playerHeight;
    float prevPlayerHeight;
    float eyeHeight;
    std::uint8_t selectedBlock = 1;

    // --- Player Flags ---
    bool isOnGround = false;
    bool isDamageable = false;

    bool isRunning = false;
    bool isFlying = false;
    bool isSwimming = false;
    bool isWalking = false;
    
    bool isSurvival = false;
    bool isCreative = false;
    bool isSpectator = false;

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


    glm::vec3 position;
    glm::vec3 prevPosition;
    glm::vec3 velocity{0.0f};
    glm::vec3 pendingMovement = glm::vec3(0.0f);

  private:
    // -- Player Model ---
    struct BodyPart {
        std::unique_ptr<Cube> cube;
        glm::vec3 offset;
        glm::mat4 transform = glm::mat4(1.0f);
    };
    std::vector<BodyPart> bodyParts;

    glm::vec3 armOffset = glm::vec3(0.3f, -0.792f, -0.5f); // Right, down, forward

    std::unique_ptr<Texture> skinTexture;
    GLuint skinVAO;

    glm::mat4 modelMat;


    float ExtentX = 0.4f;
    float ExtentY = 0.4f;
    AABB aabb;




    std::optional<std::pair<glm::ivec3, glm::ivec3>> raycastVoxelWithNormal(ChunkManager &chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance);
    std::optional<glm::ivec3> raycastVoxel(ChunkManager &chunkManager, glm::vec3 rayOrigin, glm::vec3 rayDirection, float maxDistance);
    void handleCollisions(glm::vec3 &newPosition, glm::vec3 &velocity, const glm::vec3 &oldPosition, ChunkManager &chunkManager);
    bool isCollidingAt(const glm::vec3 &pos, ChunkManager &chunkManager);



    CameraController camCtrl;
    void setupBodyParts();
};
