#include "Cube.h"
#include <iostream>
#include "defines.h"
#include "logger.hpp"

Cube::Cube(glm::vec3 size, BodyPartType type) : size_(size) {
    float halfX = size.x / 2.0f;
    float yBase = 0.0f;
    float yTop = size.y;
    float halfZ = size.z / 2.0f;

    TextureRegion frontRegion, topRegion, bottomRegion, rightRegion, leftRegion, backRegion;
    switch (type) {
        case BodyPartType::HEAD:
            topRegion    = {T(8, 0),   T(16, 8)};
            bottomRegion = {T(16, 0),  T(24, 8)};
            rightRegion  = {T(0, 8),   T(8, 16)};
            frontRegion  = {T(8, 8),   T(16, 16)};
            leftRegion   = {T(16, 8),  T(24, 16)};
            backRegion   = {T(24, 8),  T(32, 16)};
            break;

        case BodyPartType::TORSO:
            topRegion    = {T(20, 16), T(28, 20)};
            bottomRegion = {T(28, 16), T(36, 20)};
            rightRegion  = {T(16, 20), T(20, 32)};
            frontRegion  = {T(20, 20), T(28, 32)};
            leftRegion   = {T(28, 20), T(32, 32)};
            backRegion   = {T(32, 20), T(40, 32)};
            break;

        case BodyPartType::RIGHT_ARM:
            topRegion    = {T(44, 16), T(48, 20)};
            bottomRegion = {T(48, 16), T(52, 20)};
            rightRegion  = {T(40, 20), T(44, 32)};
            frontRegion  = {T(44, 20), T(48, 32)};
            leftRegion   = {T(48, 20), T(52, 32)};
            backRegion   = {T(52, 20), T(56, 32)};
            break;

        case BodyPartType::LEFT_ARM:
            // Classic skin has no separate left arm; re-use right arm's texture or fallback.
            // 64x64 skins with extra data use this area:
            topRegion    = {T(36, 48), T(40, 52)};
            bottomRegion = {T(40, 48), T(44, 52)};
            rightRegion  = {T(32, 52), T(36, 64)};
            frontRegion  = {T(36, 52), T(40, 64)};
            leftRegion   = {T(40, 52), T(44, 64)};
            backRegion   = {T(44, 52), T(48, 64)};
            break;

        case BodyPartType::RIGHT_LEG:
            topRegion    = {T(4, 16),  T(8, 20)};
            bottomRegion = {T(8, 16),  T(12, 20)};
            rightRegion  = {T(0, 20),  T(4, 32)};
            frontRegion  = {T(4, 20),  T(8, 32)};
            leftRegion   = {T(8, 20),  T(12, 32)};
            backRegion   = {T(12, 20), T(16, 32)};
            break;

        case BodyPartType::LEFT_LEG:
            topRegion    = {T(4, 48),  T(8, 52)};
            bottomRegion = {T(8, 48),  T(12, 52)};
            rightRegion  = {T(0, 52),  T(4, 64)};
            frontRegion  = {T(4, 52),  T(8, 64)};
            leftRegion   = {T(8, 52),  T(12, 64)};
            backRegion   = {T(12, 52), T(16, 64)};
            break;
    }


    // Update vertices with full texture regions
    vertices = {
        // Front face (z = halfZ, facing +z)
        -halfX, yBase, halfZ, frontRegion.topLeft.x, frontRegion.bottomRight.y,    // Bottom-left
        halfX, yBase, halfZ, frontRegion.bottomRight.x, frontRegion.bottomRight.y, // Bottom-right
        halfX, yTop, halfZ, frontRegion.bottomRight.x, frontRegion.topLeft.y,      // Top-right
        -halfX, yBase, halfZ, frontRegion.topLeft.x, frontRegion.bottomRight.y,    // Bottom-left
        halfX, yTop, halfZ, frontRegion.bottomRight.x, frontRegion.topLeft.y,      // Top-right
        -halfX, yTop, halfZ, frontRegion.topLeft.x, frontRegion.topLeft.y,         // Top-left

        // Back face (z = -halfZ, facing -z)
        -halfX, yBase, -halfZ, backRegion.bottomRight.x, backRegion.bottomRight.y, // Bottom-left
        halfX, yBase, -halfZ, backRegion.topLeft.x, backRegion.bottomRight.y,      // Bottom-right
        halfX, yTop, -halfZ, backRegion.topLeft.x, backRegion.topLeft.y,           // Top-right
        -halfX, yBase, -halfZ, backRegion.bottomRight.x, backRegion.bottomRight.y, // Bottom-left
        halfX, yTop, -halfZ, backRegion.topLeft.x, backRegion.topLeft.y,           // Top-right
        -halfX, yTop, -halfZ, backRegion.bottomRight.x, backRegion.topLeft.y,      // Top-left

        // Left face (x = -halfX, facing -x)
        -halfX, yBase, halfZ, leftRegion.bottomRight.x, leftRegion.bottomRight.y, // Bottom-front
        -halfX, yBase, -halfZ, leftRegion.topLeft.x, leftRegion.bottomRight.y,    // Bottom-back
        -halfX, yTop, -halfZ, leftRegion.topLeft.x, leftRegion.topLeft.y,         // Top-back
        -halfX, yBase, halfZ, leftRegion.bottomRight.x, leftRegion.bottomRight.y, // Bottom-front
        -halfX, yTop, -halfZ, leftRegion.topLeft.x, leftRegion.topLeft.y,         // Top-back
        -halfX, yTop, halfZ, leftRegion.bottomRight.x, leftRegion.topLeft.y,      // Top-front

        // Right face (x = halfX, facing +x)
        halfX, yBase, -halfZ, rightRegion.topLeft.x, rightRegion.bottomRight.y,    // Bottom-back
        halfX, yBase, halfZ, rightRegion.bottomRight.x, rightRegion.bottomRight.y, // Bottom-front
        halfX, yTop, halfZ, rightRegion.bottomRight.x, rightRegion.topLeft.y,      // Top-front
        halfX, yBase, -halfZ, rightRegion.topLeft.x, rightRegion.bottomRight.y,    // Bottom-back
        halfX, yTop, halfZ, rightRegion.bottomRight.x, rightRegion.topLeft.y,      // Top-front
        halfX, yTop, -halfZ, rightRegion.topLeft.x, rightRegion.topLeft.y,         // Top-back

        // Top face (y = yTop, facing +y)
        -halfX, yTop, halfZ, topRegion.topLeft.x, topRegion.bottomRight.y,    // Front-left
        halfX, yTop, halfZ, topRegion.bottomRight.x, topRegion.bottomRight.y, // Front-right
        halfX, yTop, -halfZ, topRegion.bottomRight.x, topRegion.topLeft.y,    // Back-right
        -halfX, yTop, halfZ, topRegion.topLeft.x, topRegion.bottomRight.y,    // Front-left
        halfX, yTop, -halfZ, topRegion.bottomRight.x, topRegion.topLeft.y,    // Back-right
        -halfX, yTop, -halfZ, topRegion.topLeft.x, topRegion.topLeft.y,       // Back-left

        // Bottom face (y = yBase, facing -y)
        -halfX, yBase, -halfZ, bottomRegion.topLeft.x, bottomRegion.bottomRight.y,    // Back-left
        halfX, yBase, -halfZ, bottomRegion.bottomRight.x, bottomRegion.bottomRight.y, // Back-right
        halfX, yBase, halfZ, bottomRegion.bottomRight.x, bottomRegion.topLeft.y,      // Front-right
        -halfX, yBase, -halfZ, bottomRegion.topLeft.x, bottomRegion.bottomRight.y,    // Back-left
        halfX, yBase, halfZ, bottomRegion.bottomRight.x, bottomRegion.topLeft.y,      // Front-right
        -halfX, yBase, halfZ, bottomRegion.topLeft.x, bottomRegion.topLeft.y          // Front-left
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
    glNamedBufferData(VBO, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Bind vertex buffer to VAO at binding index 0
    glVertexArrayVertexBuffer(VAO, 0, VBO, 0, 5 * sizeof(float));

    // Configure vertex attributes
    glEnableVertexArrayAttrib(VAO, 0);
    glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(VAO, 0, 0); // Bind attribute 0 to binding index 0

    glEnableVertexArrayAttrib(VAO, 1);
    glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glVertexArrayAttribBinding(VAO, 1, 0); // Bind attribute 1 to binding index 0
}

void Cube::render(const glm::mat4 &transform) {
    glBindVertexArray(VAO);
    DrawArraysWrapper(GL_TRIANGLES, 0, 36); // 36 vertices for 12 triangles
    glBindVertexArray(0);

#if defined(DEBUG)
    float halfX = size_.x / 2.0f;
    float yBase = 0.0f;
    float yTop = size_.y;
    float halfZ = size_.z / 2.0f;
    glm::vec3 halfExtents(halfX, (yTop - yBase) / 2.0f, halfZ);
    getDebugDrawer().addOBB(transform, halfExtents, glm::vec3(1.0f, 0.0f, 0.0f));
#endif

}
