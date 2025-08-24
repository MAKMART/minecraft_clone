#pragma once
#include "core/aabb.hpp"
#include <vector>
#include "shader.hpp"
#include <memory>

class DebugDrawer {
  public:
    DebugDrawer();
    ~DebugDrawer();

    void addAABB(const AABB &box, const glm::vec3 &color);
    void addOBB(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec3& color);
    void draw(const glm::mat4 &viewProj);

  private:
    struct DebugAABB {
        AABB box;
        glm::vec3 color;
        DebugAABB(const AABB& b, const glm::vec3& c) : box(b), color(c) {}
    };

    struct DebugOBB {
        glm::mat4 transform;
        glm::vec3 halfExtents;
        glm::vec3 color;
        DebugOBB(const glm::mat4& t, const glm::vec3& h, const glm::vec3& c)
            : transform(t), halfExtents(h), color(c) {}
    };
    std::vector<DebugAABB> aabbs;
    std::vector<DebugOBB> obbs;
    std::vector<glm::vec3> vertices;
    Shader* shader;
    GLuint vao = 0;
    GLuint vbo = 0;

    void initGLResources();
    void destroyGLResources();
    void checkGLError(const std::string& operation);
};
