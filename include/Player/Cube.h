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
class Cube {
public:
    Cube(glm::vec3 size, BodyPartType type);  // Added parameters for skin mapping
    ~Cube(void);
    void render(unsigned int shaderProgram, const glm::mat4& transform);
    struct TextureRegion {
	glm::vec2 topLeft;     // (x1, y1)
	glm::vec2 bottomRight; // (x2, y2)
    };

private:
    GLuint SSBO = 0, VAO, VBO;
    struct Side {
	int32_t packed;
	Side(int vx, int vy, int vz, int tex_u, int tex_v) {
	    packed = (vx & 0x3FF) | ((vy & 0x3FF) << 10) | ((vz & 0x3FF) << 20) | (tex_u & 0x3FF) | ((tex_v & 0x3FF) << 10);
	}
    };
    std::vector<Side> sides;
    std::vector<float> vertices;
    void setupBuffers(void);
};
