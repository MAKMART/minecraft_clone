#include "AABBDebugDrawer.h"
#include <glm/gtc/type_ptr.hpp>
#include "defines.h"
#include <vector>

AABBDebugDrawer::AABBDebugDrawer() {
    initGLResources();
    shader = std::make_unique<Shader>("shaders/debug_vert.glsl", "shaders/debug_frag.glsl");
}
AABBDebugDrawer::~AABBDebugDrawer() {
    destroyGLResources();
}
void AABBDebugDrawer::initGLResources() {
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);

    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(float) * 6);

    glEnableVertexArrayAttrib(vao, 0); // position
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glEnableVertexArrayAttrib(vao, 1); // color
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3);
    glVertexArrayAttribBinding(vao, 1, 0);
}

void AABBDebugDrawer::destroyGLResources() {
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
}
void AABBDebugDrawer::addAABB(const AABB &box, const glm::vec3 &color) {
    boxes.push_back({box, color});
}

void AABBDebugDrawer::draw(const glm::mat4 &viewProj) {
    shader->use();
    glBindVertexArray(vao);

    std::vector<float> lineData;

    for (const auto &debugBox : boxes) {
        const glm::vec3 &min = debugBox.box.min;
        const glm::vec3 &max = debugBox.box.max;

        glm::vec3 corners[8] = {
            {min.x, min.y, min.z},
            {max.x, min.y, min.z},
            {max.x, max.y, min.z},
            {min.x, max.y, min.z},
            {min.x, min.y, max.z},
            {max.x, min.y, max.z},
            {max.x, max.y, max.z},
            {min.x, max.y, max.z},
        };

        GLuint indices[] = {
            0, 1, 1, 2, 2, 3, 3, 0,
            4, 5, 5, 6, 6, 7, 7, 4,
            0, 4, 1, 5, 2, 6, 3, 7};

        for (int i = 0; i < 24; i++) {
            glm::vec3 pos = corners[indices[i]];
            lineData.push_back(pos.x);
            lineData.push_back(pos.y);
            lineData.push_back(pos.z);
            lineData.push_back(debugBox.color.r);
            lineData.push_back(debugBox.color.g);
            lineData.push_back(debugBox.color.b);
        }
        shader->setMat4("MVP", viewProj);
    }

    // Upload all bounding box lines once per frame
    glNamedBufferData(vbo, lineData.size() * sizeof(float), lineData.data(), GL_DYNAMIC_DRAW);

    // Draw all lines at once: each line has 2 vertices, total vertices = lineData.size()/6
    DrawArraysWrapper(GL_LINES, 0, static_cast<GLsizei>(lineData.size() / 6));

    glBindVertexArray(0);
    boxes.clear();
}
