#pragma once
#include "AABB.h"
#include <vector>
#include "Shader.h"
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
    std::vector<DebugAABB> boxes; // Stores AABBs for rendering
    std::vector<DebugOBB> obbs;   // Stores OBBs for rendering
    std::unique_ptr<Shader> shader; // Debug shader for rendering
    GLuint vao = 0; // Vertex Array Object for wireframe cube
    GLuint vbo = 0; // Vertex Buffer Object for wireframe cube

    void initGLResources();   // Initialize OpenGL resources (VAO, VBO, shader)
    void destroyGLResources(); // Clean up OpenGL resources
    void checkGLError(const std::string& operation); // Check for OpenGL errors
};
