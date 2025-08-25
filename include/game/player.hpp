#pragma once
#include <memory>
#include <optional>
#include <GL/glew.h>
#include "core/defines.hpp"
#include "core/input_manager.hpp"
#include "chunk/chunk_manager.hpp"
#include "core/camera_controller.hpp"
#include "graphics/cube.hpp"
#include "core/aabb.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/position.hpp"
#include "game/ecs/components/velocity.hpp"
#include "game/ecs/components/player_state.hpp"
#include "game/ecs/components/player_mode.hpp"
#include "game/ecs/components/input.hpp"
#include "game/ecs/components/collider.hpp"

class Player {
  public:

	  template<typename... Components>
		  Player(ECS<Components...>& ecs, glm::vec3 spawnPos, InputManager& _input)
		  : playerHeight(1.8f), input(_input), prevPlayerHeight(playerHeight), skinTexture(std::make_unique<Texture>(DEFAULT_SKIN_DIRECTORY, GL_RGBA, GL_CLAMP_TO_EDGE, GL_NEAREST))
		  {
			  log::info("Initializing Player...");
			  self = ecs.createEntity();

			  ecs.addComponent(self, Position{spawnPos});
			  position = ecs.template getComponent<Position>(self);


			  ecs.addComponent(self, Velocity{});
			  velocity = ecs.template getComponent<Velocity>(self);

			  ecs.addComponent(self, Collider{glm::vec3(ExtentX, playerHeight / 2.0f, ExtentY)});
			  collider = ecs.template getComponent<Collider>(self);

			  ecs.addComponent(self, InputComponent{});
			  input_comp = ecs.template getComponent<InputComponent>(self);

			  ecs.addComponent(self, PlayerState{});
			  state = ecs.template getComponent<PlayerState>(self);

			  ecs.addComponent(self, PlayerMode{});
			  mode = ecs.template getComponent<PlayerMode>(self);


			  // Other init stuff
			  eyeHeight = playerHeight * 0.9f;
			  scaleFactor = 0.076f;
			  lastScaleFactor = scaleFactor;
			  camCtrl = CameraController(position->value + glm::vec3(0, eyeHeight, 0), glm::quat(1,0,0,0));



			  setupBodyParts();
			  camCtrl.setOrbitDistance(6.0f);
			  glCreateVertexArrays(1, &skinVAO);
			  glBindVertexArray(skinVAO);
		  }

    ~Player() {
	    glDeleteVertexArrays(1, &skinVAO);
    }


    enum ACTION { BREAK_BLOCK, PLACE_BLOCK };

    const glm::vec3& getPos() const { return position->value; }
    // Non-const &: allows modification
    CameraController& getCameraController() { return camCtrl; }
    // Const &: allows read-only access
    const CameraController& getCameraController() const { return camCtrl; }
    const Camera& getCamera() const { return camCtrl.getCamera(); }

    glm::mat4 getViewMatrix() const { return camCtrl.getViewMatrix(); }
    glm::mat4 getProjectionMatrix() const { return camCtrl.getProjectionMatrix(); };
    glm::vec3 getCameraFront() const { return camCtrl.getFront(); };
    glm::vec3 getCameraRight() const { return camCtrl.getRight(); };
    glm::vec3 getCameraUp() const { return camCtrl.getUp(); };
    const glm::mat4& getModelMatrix() const { return modelMat; }

    // Non-const ref can be changed
    glm::vec3& getArmOffset() { return armOffset; };


    bool isCameraThirdPerson() const { return camCtrl.isThirdPersonMode(); }

    void update(float deltaTime, ChunkManager &chunkManager);
    void render(const Shader &shader);
    void loadSkin(const std::string &path);
    void loadSkin(const std::filesystem::path &path);

    bool is_on_ground() const {
	    return collider->is_on_ground;
    }

    bool isRunning() const {
	    return state->current == PlayerMovementState::Running;
    }


    bool isWalking() const {
	    return state->current == PlayerMovementState::Walking;
    }


    bool isIdling() const {
	    return state->current == PlayerMovementState::Idle;
    }


    bool isCrouching() const {
	    return state->current == PlayerMovementState::Crouching;
    }


    bool isFlying() const {
	    return state->current == PlayerMovementState::Flying;
    }


    bool isJumping() const {
	    return state->current == PlayerMovementState::Jumping;
    }


    bool isSwimming() const {
	    return state->current == PlayerMovementState::Swimming;
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch);
    void processMouseInput(ChunkManager &chunkManager);
    void processKeyInput();
    void processMouseScroll(float yoffset);


    void setPos(glm::vec3 newPos);

    const glm::vec3& getVelocity() const {
	    return velocity->value;
    }

    AABB getAABB() const {
        return collider->getBoundingBoxAt(position->value);
    }

    bool isInsidePlayerBoundingBox(const glm::vec3 &checkPos) const;
    void breakBlock(ChunkManager &chunkManager);
    void placeBlock(ChunkManager &chunkManager);


    const char* getMode() const {
	    switch (mode->mode) {
		    case Type::SURVIVAL:
			    return "SURVIVAL";
			    break;
		    case Type::CREATIVE:
			    return "CREATIVE";
			    break;
		    case Type::SPECTATOR:
			    return "SPECTATOR";
			    break;
		    default:
			    return "UNHANDLED";
	    }
    }


    const char* getMovementState() const {
	    switch (state->current) {
		    case PlayerMovementState::Idle:
			    return "IDLE";
			    break;
		    case PlayerMovementState::Walking:
			    return "WALKING";
			    break;
		    case PlayerMovementState::Running:
			    return "RUNNNIG";
			    break;
		    case PlayerMovementState::Jumping:
			    return "JUMPING";
			    break;
		    case PlayerMovementState::Crouching:
			    return "CROUCHING";
			    break;
		    case PlayerMovementState::Flying:
			    return "FLYING";
			    break;
		    case PlayerMovementState::Swimming:
			    return "SWIMMING";
			    break;
		    default:
			    return "UNHANDLED";
	    }
    }


    void toggleCameraMode() {
        camCtrl.toggleThirdPersonMode();
    }

    void updateCamPos() {
        if (camCtrl.isThirdPersonMode())
            camCtrl.setOrbitTarget(position->value + glm::vec3(0, eyeHeight, 0));
        else
            camCtrl.setPosition(position->value + glm::vec3(0, eyeHeight, 0));
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
    bool isDamageable = false;

    bool canPlaceBlocks = false;
    bool canBreakBlocks = false;
    bool renderSkin = false;
    bool canFly = true;
    bool canWalk = true;
    bool canRun = true;
    bool canJump = true;
    bool canCrouch = true;

    bool hasInfiniteBlocks = false;


  private:
    Entity self;
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


    Position* position;
    Velocity* velocity;
    Collider* collider;
    InputComponent* input_comp;
    PlayerState* state;
    PlayerMode* mode;
    CameraController camCtrl;


    float ExtentX = 0.4f;
    float ExtentY = 0.4f;


    InputManager& input;


    void setupBodyParts();
};
