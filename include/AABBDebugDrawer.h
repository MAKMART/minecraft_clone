#pragma once
#include "AABB.h"
#include <vector>
#include "Shader.h"
#include <memory>

class AABBDebugDrawer {
  public:
    AABBDebugDrawer();
    ~AABBDebugDrawer();

    void addAABB(const AABB &box, const glm::vec3 &color);
    void draw(const glm::mat4 &viewProj);

  private:
    struct DebugAABB {
        AABB box;
        glm::vec3 color;
    };

    std::vector<DebugAABB> boxes;

    std::unique_ptr<Shader> shader;
    GLuint vao = 0, vbo = 0;

    void initGLResources();
    void destroyGLResources();
};
