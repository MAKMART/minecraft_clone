#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

enum class BodyPartType {
	HEAD,
	TORSO,
	RIGHT_ARM,
	LEFT_ARM,
	RIGHT_LEG,
	LEFT_LEG
};
class Cube
{
      public:
	Cube(glm::vec3 size, BodyPartType type); // Added parameters for skin mapping
	~Cube();
	void render(const glm::mat4& transform);
	struct TextureRegion {
		glm::vec2 topLeft;     // normalized UV
		glm::vec2 bottomRight; // normalized UV
	};

	glm::vec3& getSize()
	{
		return size_;
	}
	GLuint& getSSBO()
	{
		return SSBO;
	}

      private:
	float            texScale = 1.0f / 64.0f;
	inline glm::vec2 T(float x, float y)
	{
		return glm::vec2(x * texScale, y * texScale);
	}

	GLuint SSBO = 0;
	struct Face {
		std::uint32_t position; // packed as before
		std::uint32_t tex_info; // packed info for face region + face_id

		Face(int vx, int vy, int vz, int u_base, int v_base, int face)
		{
			position = (vx & 0x3FF) | ((vy & 0x3FF) << 10) | ((vz & 0x3FF) << 20);
			tex_info = (u_base & 0x3FF) | ((v_base & 0x3FF) << 10) | ((face & 0x7) << 20);
		}
	};
	glm::vec3 size_;
	float     halfX;
	float     yBase;
	float     yTop;
	float     halfZ;

	std::vector<Face> faces;
	void              sendData(void);
};
