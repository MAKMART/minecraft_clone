#include "Cube.h"
#include <iostream>
#include "defines.h"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_int2.hpp"
#include "logger.hpp"

Cube::Cube(glm::vec3 size, BodyPartType type) : size_(size), halfX(size.x / 2.0f), yBase(0.0f), yTop(size.y), halfZ(size.z / 2.0f) {

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

    auto pushFace = [this](glm::vec3 offset, glm::ivec2 uv, int face) {
        // Scale offsets to fit 10-bit range (0–1023)
        float scale = 1023.0f / std::max(std::max(size_.x, size_.y), size_.z); // Normalize to largest dimension
        faces.emplace_back(Face(offset.x * scale, offset.y * scale, offset.z * scale, uv.x, uv.y, face));
    };

    const float centerY = (yTop + yBase) / 2.0f;
    const float height = yTop - yBase;

    // These are offsets from the center, not absolute
    pushFace(glm::vec3(0.0f, 0.0f,  halfZ), glm::ivec2(frontRegion.topLeft.x * 64, frontRegion.topLeft.y * 64), 0); // Front
    pushFace(glm::vec3(0.0f, 0.0f, -halfZ), glm::ivec2(backRegion.topLeft.x * 64, backRegion.topLeft.y * 64),  1); // Back
    pushFace(glm::vec3(-halfX, 0.0f, 0.0f), glm::ivec2(leftRegion.topLeft.x * 64, leftRegion.topLeft.y * 64),  2); // Left
    pushFace(glm::vec3( halfX, 0.0f, 0.0f), glm::ivec2(rightRegion.topLeft.x * 64, rightRegion.topLeft.y * 64), 3); // Right
    pushFace(glm::vec3(0.0f,  height / 2.0f, 0.0f), glm::ivec2(topRegion.topLeft.x * 64, topRegion.topLeft.y * 64),   4); // Top
    pushFace(glm::vec3(0.0f, -height / 2.0f, 0.0f), glm::ivec2(bottomRegion.topLeft.x * 64, bottomRegion.topLeft.y * 64), 5); // Bottom

    sendData();
}
Cube::~Cube(void) {
    if (SSBO)
        glDeleteBuffers(1, &SSBO);
}
void Cube::sendData(void) {
    glCreateBuffers(1, &SSBO);
    glNamedBufferData(SSBO, faces.size() * sizeof(Face), faces.data(), GL_DYNAMIC_DRAW);

}
void Cube::render(const glm::mat4 &transform) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, SSBO);
    DrawArraysWrapper(GL_TRIANGLES, 0, faces.size() * 6);

#if defined(DEBUG)
    glm::vec3 halfExtents(halfX, (yTop - yBase) / 2.0f, halfZ);
    getDebugDrawer().addOBB(transform, halfExtents, glm::vec3(1.0f, 0.0f, 0.0f));
#endif

}
