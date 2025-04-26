#include "Cube.h"
#include <iostream>
#include "defines.h"

Cube::Cube(glm::vec3 size, BodyPartType type, glm::vec2 texOffset) {
    float halfX = size.x / 2.0f;
    float yBase = 0.0f;
    float yTop = size.y;
    float halfZ = size.z / 2.0f;
    float texScale = 1.0f / 64.0f;

    (void)texOffset;
    // Define texture regions for each face
    TextureRegion frontRegion, topRegion, bottomRegion, rightRegion, leftRegion, backRegion;
    switch (type) {
        case BodyPartType::HEAD:
            topRegion    = { glm::vec2(8,  0),  glm::vec2(15, 7)  };
            bottomRegion = { glm::vec2(16, 0),  glm::vec2(23, 7)  };
            rightRegion  = { glm::vec2(0,  8),  glm::vec2(7,  15) };
            frontRegion  = { glm::vec2(8,  8),  glm::vec2(15, 15) };
            leftRegion   = { glm::vec2(16, 8),  glm::vec2(23, 15) };
            backRegion   = { glm::vec2(24, 8),  glm::vec2(31, 15) };
            break;
        case BodyPartType::TORSO:
            topRegion    = { glm::vec2(20, 16), glm::vec2(27, 19) };
            bottomRegion = { glm::vec2(28, 16), glm::vec2(35, 19) };
            rightRegion  = { glm::vec2(16, 20), glm::vec2(19, 31) };
            frontRegion  = { glm::vec2(20, 20), glm::vec2(27, 31) };
            leftRegion   = { glm::vec2(28, 20), glm::vec2(31, 31) };
            backRegion   = { glm::vec2(32, 20), glm::vec2(39, 31) };
            break;
        case BodyPartType::RIGHT_ARM:
            topRegion    = { glm::vec2(44, 16), glm::vec2(47, 19) };
            bottomRegion = { glm::vec2(48, 16), glm::vec2(51, 19) };
            rightRegion  = { glm::vec2(40, 20), glm::vec2(43, 31) };
            frontRegion  = { glm::vec2(44, 20), glm::vec2(47, 31) };
            leftRegion   = { glm::vec2(48, 20), glm::vec2(51, 31) };
            backRegion   = { glm::vec2(52, 20), glm::vec2(55, 31) };
            break;
        case BodyPartType::LEFT_ARM:
            topRegion    = { glm::vec2(36, 52), glm::vec2(39, 63) };
            bottomRegion = { glm::vec2(40, 48), glm::vec2(43, 51) };
            rightRegion  = { glm::vec2(32, 52), glm::vec2(35, 63) };
            frontRegion  = { glm::vec2(36, 52), glm::vec2(39, 63) };
            leftRegion   = { glm::vec2(40, 52), glm::vec2(43, 63) };
            backRegion   = { glm::vec2(44, 52), glm::vec2(48, 63) };
            break;
        case BodyPartType::RIGHT_LEG:
            topRegion    = { glm::vec2(4,  16), glm::vec2(7,  19) };
            bottomRegion = { glm::vec2(8,  16), glm::vec2(11, 19) };
            rightRegion  = { glm::vec2(0,  20), glm::vec2(3,  31) };
            frontRegion  = { glm::vec2(4,  20), glm::vec2(7,  31) };
            leftRegion   = { glm::vec2(8,  20), glm::vec2(11, 31) };
            backRegion   = { glm::vec2(12, 20), glm::vec2(15, 31) };
            break;
        case BodyPartType::LEFT_LEG:
            topRegion    = { glm::vec2(20, 48), glm::vec2(23, 51) };
            bottomRegion = { glm::vec2(24, 48), glm::vec2(27, 51) };
            rightRegion  = { glm::vec2(16, 52), glm::vec2(19, 63) };
            frontRegion  = { glm::vec2(20, 52), glm::vec2(23, 63) };
            leftRegion   = { glm::vec2(24, 52), glm::vec2(27, 63) };
            backRegion   = { glm::vec2(28, 52), glm::vec2(31, 63) };
            break;
    }

    // Update vertices with full texture regions
    vertices = {
        // Front face (z = halfZ, facing +z)
        -halfX, yBase,  halfZ,  frontRegion.topLeft.x * texScale,     frontRegion.bottomRight.y * texScale, // Bottom-left
         halfX, yBase,  halfZ,  frontRegion.bottomRight.x * texScale, frontRegion.bottomRight.y * texScale, // Bottom-right
         halfX, yTop,   halfZ,  frontRegion.bottomRight.x * texScale, frontRegion.topLeft.y * texScale,     // Top-right
        -halfX, yBase,  halfZ,  frontRegion.topLeft.x * texScale,     frontRegion.bottomRight.y * texScale, // Bottom-left
         halfX, yTop,   halfZ,  frontRegion.bottomRight.x * texScale, frontRegion.topLeft.y * texScale,     // Top-right
        -halfX, yTop,   halfZ,  frontRegion.topLeft.x * texScale,     frontRegion.topLeft.y * texScale,     // Top-left

        // Back face (z = -halfZ, facing -z)
        -halfX, yBase, -halfZ,  backRegion.bottomRight.x * texScale, backRegion.bottomRight.y * texScale, // Bottom-left
         halfX, yBase, -halfZ,  backRegion.topLeft.x * texScale,     backRegion.bottomRight.y * texScale, // Bottom-right
         halfX, yTop,  -halfZ,  backRegion.topLeft.x * texScale,     backRegion.topLeft.y * texScale,     // Top-right
        -halfX, yBase, -halfZ,  backRegion.bottomRight.x * texScale, backRegion.bottomRight.y * texScale, // Bottom-left
         halfX, yTop,  -halfZ,  backRegion.topLeft.x * texScale,     backRegion.topLeft.y * texScale,     // Top-right
        -halfX, yTop,  -halfZ,  backRegion.bottomRight.x * texScale, backRegion.topLeft.y * texScale,     // Top-left

        // Left face (x = -halfX, facing -x)
        -halfX, yBase,  halfZ,  leftRegion.bottomRight.x * texScale, leftRegion.bottomRight.y * texScale, // Bottom-front
        -halfX, yBase, -halfZ,  leftRegion.topLeft.x * texScale,     leftRegion.bottomRight.y * texScale, // Bottom-back
        -halfX, yTop,  -halfZ,  leftRegion.topLeft.x * texScale,     leftRegion.topLeft.y * texScale,     // Top-back
        -halfX, yBase,  halfZ,  leftRegion.bottomRight.x * texScale, leftRegion.bottomRight.y * texScale, // Bottom-front
        -halfX, yTop,  -halfZ,  leftRegion.topLeft.x * texScale,     leftRegion.topLeft.y * texScale,     // Top-back
        -halfX, yTop,   halfZ,  leftRegion.bottomRight.x * texScale, leftRegion.topLeft.y * texScale,     // Top-front

        // Right face (x = halfX, facing +x)
         halfX, yBase, -halfZ,  rightRegion.topLeft.x * texScale,     rightRegion.bottomRight.y * texScale, // Bottom-back
         halfX, yBase,  halfZ,  rightRegion.bottomRight.x * texScale, rightRegion.bottomRight.y * texScale, // Bottom-front
         halfX, yTop,   halfZ,  rightRegion.bottomRight.x * texScale, rightRegion.topLeft.y * texScale,     // Top-front
         halfX, yBase, -halfZ,  rightRegion.topLeft.x * texScale,     rightRegion.bottomRight.y * texScale, // Bottom-back
         halfX, yTop,   halfZ,  rightRegion.bottomRight.x * texScale, rightRegion.topLeft.y * texScale,     // Top-front
         halfX, yTop,  -halfZ,  rightRegion.topLeft.x * texScale,     rightRegion.topLeft.y * texScale,     // Top-back

        // Top face (y = yTop, facing +y)
        -halfX, yTop,  halfZ,  topRegion.topLeft.x * texScale,      topRegion.bottomRight.y * texScale, // Front-left
         halfX, yTop,  halfZ,  topRegion.bottomRight.x * texScale,  topRegion.bottomRight.y * texScale, // Front-right
         halfX, yTop, -halfZ,  topRegion.bottomRight.x * texScale,  topRegion.topLeft.y * texScale,     // Back-right
        -halfX, yTop,  halfZ,  topRegion.topLeft.x * texScale,      topRegion.bottomRight.y * texScale, // Front-left
         halfX, yTop, -halfZ,  topRegion.bottomRight.x * texScale,  topRegion.topLeft.y * texScale,     // Back-right
        -halfX, yTop, -halfZ,  topRegion.topLeft.x * texScale,      topRegion.topLeft.y * texScale,     // Back-left

        // Bottom face (y = yBase, facing -y)
        -halfX, yBase, -halfZ,  bottomRegion.topLeft.x * texScale,     bottomRegion.bottomRight.y * texScale, // Back-left
         halfX, yBase, -halfZ,  bottomRegion.bottomRight.x * texScale, bottomRegion.bottomRight.y * texScale, // Back-right
         halfX, yBase,  halfZ,  bottomRegion.bottomRight.x * texScale, bottomRegion.topLeft.y * texScale,     // Front-right
        -halfX, yBase, -halfZ,  bottomRegion.topLeft.x * texScale,     bottomRegion.bottomRight.y * texScale, // Back-left
         halfX, yBase,  halfZ,  bottomRegion.bottomRight.x * texScale, bottomRegion.topLeft.y * texScale,     // Front-right
        -halfX, yBase,  halfZ,  bottomRegion.topLeft.x * texScale,     bottomRegion.topLeft.y * texScale      // Front-left
    };

    setupBuffers();
}
Cube::~Cube(void) {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}
void Cube::setupBuffers(void) {
    // Create VAO and VBO
    glCreateVertexArrays(1, &VAO);
    glCreateBuffers(1, &VBO);

    // Upload vertex data to the VBO
    glNamedBufferData(VBO, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    // Bind vertex buffer to VAO at binding index 0
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 5 * sizeof(float));

    // Configure vertex attributes
    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 0, 0); // Bind attribute 0 to binding index 0

    glEnableVertexArrayAttrib(VAO, 1);
    glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(VAO, 1, 0); // Bind attribute 1 to binding index 0

    // Debugging info (optional)
    //std::cout << "Cube vertex count: " << vertices.size() / 5
    //        << ", size in bytes: " << vertices.size() * sizeof(float) << " B\n";
}

void Cube::render(unsigned int shaderProgram, const glm::mat4& transform) {
    //glUseProgram(shaderProgram);
    
    GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
    if (modelLoc == -1) {
        std::cerr << "Error: Could not find uniform location for 'model' in shader program.\n";
        return;
    }
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(transform));
    
    glBindVertexArray(VAO);
    DrawArraysWrapper(GL_TRIANGLES, 0, 36); // 36 vertices for 12 triangles
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error in Cube::render: " << err << std::endl;
    }
    glBindVertexArray(0);
}
