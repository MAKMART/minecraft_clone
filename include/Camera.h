#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GL/glew.h>
#include <array>
#include "AABB.h"
enum class Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN,
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;

class Camera
{
public:
	// camera attributes
	glm::vec3 Position;
	glm::quat orientation;
	glm::vec3 WorldUp;
	// Euler angles
	float Yaw;
	float Pitch;
	// camera options
	float MovementSpeed = 2.5f;
	float MouseSensitivity = 0.2f;
	float FOV = 90.0f;
	float aspectRatio = 16.0f / 9.0f;
	float FAR_PLANE = 1000.0f;
	float NEAR_PLANE = 0.01f;
	bool trackMouse;

	void setMouseTracking(bool enabled) { trackMouse = enabled; }
	void setAspectRatio(float _ratio) {aspectRatio = _ratio; }


	// Third-person camera properties
	glm::vec3 Target;          // The player position (focus point)
	float Distance;            // Distance from the camera to the player (third-person only)
	bool isThirdPerson = false;        // Toggle between first-person and third-person

	void UpdateThirdPerson(const glm::vec3& target);
	void SwitchToThirdPerson(const glm::vec3& target, float distance = 5.0f, float yaw = -90.0f, float pitch = 20.0f);
	void SwitchToFirstPerson(const glm::vec3& position);

	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);

	// constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

	// returns the view matrix calculated using Euler Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix() const;


	// Returns the vector that points where the camera is facing
	glm::vec3 getFront(void) const {
	    float yawRad   = glm::radians(Yaw);
	    float pitchRad = glm::radians(Pitch);

	    return glm::normalize(glm::vec3{
		    cos(pitchRad) * cos(yawRad),
		    sin(pitchRad),
		    cos(pitchRad) * sin(yawRad)
		    });
	}
	// Returns the vector that points at the right of the camera
	glm::vec3 getRight(void) const {
	    return glm::normalize(glm::cross(getFront(), WorldUp));
	}

	// Returns the vector that points at the left of the camera
	glm::vec3 getUp(void) const {
	    return glm::normalize(glm::cross(getRight(), getFront()));
	}




	// return the projection matrix
	glm::mat4 GetProjectionMatrix() const;

	// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	glm::vec3 ProcessKeyboard(Camera_Movement direction, float deltaTime, glm::vec3 velocity);

	glm::vec3 ProcessKeyboard(Camera_Movement direction, float deltaTime, float movement_velocity);

	// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

	// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset);

	// Frustum planes
	struct FrustumPlane {
	    glm::vec3 normal;
	    float distance;
	};

	// Extract frustum planes from the view-projection matrix
	std::array<FrustumPlane, 6> extractFrustumPlanes() const {
	    glm::mat4 viewProj = GetProjectionMatrix() * GetViewMatrix();
	    std::array<FrustumPlane, 6> planes;

	    // Left plane
	    planes[0].normal.x = viewProj[0][3] + viewProj[0][0];
	    planes[0].normal.y = viewProj[1][3] + viewProj[1][0];
	    planes[0].normal.z = viewProj[2][3] + viewProj[2][0];
	    planes[0].distance = viewProj[3][3] + viewProj[3][0];

	    // Right plane
	    planes[1].normal.x = viewProj[0][3] - viewProj[0][0];
	    planes[1].normal.y = viewProj[1][3] - viewProj[1][0];
	    planes[1].normal.z = viewProj[2][3] - viewProj[2][0];
	    planes[1].distance = viewProj[3][3] - viewProj[3][0];

	    // Bottom plane
	    planes[2].normal.x = viewProj[0][3] + viewProj[0][1];
	    planes[2].normal.y = viewProj[1][3] + viewProj[1][1];
	    planes[2].normal.z = viewProj[2][3] + viewProj[2][1];
	    planes[2].distance = viewProj[3][3] + viewProj[3][1];

	    // Top plane
	    planes[3].normal.x = viewProj[0][3] - viewProj[0][1];
	    planes[3].normal.y = viewProj[1][3] - viewProj[1][1];
	    planes[3].normal.z = viewProj[2][3] - viewProj[2][1];
	    planes[3].distance = viewProj[3][3] - viewProj[3][1];

	    // Near plane
	    planes[4].normal.x = viewProj[0][3] + viewProj[0][2];
	    planes[4].normal.y = viewProj[1][3] + viewProj[1][2];
	    planes[4].normal.z = viewProj[2][3] + viewProj[2][2];
	    planes[4].distance = viewProj[3][3] + viewProj[3][2];

	    // Far plane
	    planes[5].normal.x = viewProj[0][3] - viewProj[0][2];
	    planes[5].normal.y = viewProj[1][3] - viewProj[1][2];
	    planes[5].normal.z = viewProj[2][3] - viewProj[2][2];
	    planes[5].distance = viewProj[3][3] - viewProj[3][2];

	    // Normalize planes
	    for (auto& plane : planes) {
		float length = glm::length(plane.normal);
		plane.normal /= length;
		plane.distance /= length;
	    }

	    return planes;
	}
	// Checks if a chunk is visible
	bool isChunkVisible(const AABB& bounds) const {
	    const auto planes = extractFrustumPlanes();

	    for (const auto& plane : planes) {
		glm::vec3 farthestPoint = bounds.getFarthestPoint(plane.normal);

		float distance = glm::dot(plane.normal, farthestPoint) + plane.distance;
		if (distance < 0.0f) {
		    return false; // AABB is fully outside this plane
		}
	    }

	    return true; // AABB intersects or is inside all planes
	}

};
