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
    Cube(glm::vec3 size, BodyPartType type, glm::vec2 texOffset);  // Added parameters for skin mapping
    ~Cube(void);
    void render(unsigned int shaderProgram, const glm::mat4& transform);
    struct TextureRegion {
	glm::vec2 topLeft;     // (x1, y1)
	glm::vec2 bottomRight; // (x2, y2)
    };

private:
    GLuint VAO, VBO;
    std::vector<float> vertices;
    void setupBuffers(void);
};
