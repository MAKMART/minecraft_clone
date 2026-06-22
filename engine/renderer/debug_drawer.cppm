module;
#include <glad/gl.h>
export module engine.renderer:debug_drawer;

import engine.core;
import engine.math;
// import :shader;
// import :vertex_buffer;
// import std;

using namespace engine::math;
export namespace engine::renderer {
  class DebugDrawer {
    public:
      static DebugDrawer& get() {
        static DebugDrawer instance;
        return instance;
      }

      void add_aabb(const AABB& b, vec3 color);
      void add_obb(const mat4& transform, vec3 half_extents, vec3 color);
      void add_ray(vec3 origin, vec3 d, vec3 color);
      void draw(const mat4& view_projection);

    private:
      // Make these private because a singleton class should never have the possibility to be constructed/destructed because its lifetime is managed internally
      struct DebugVertex {
        vec3 pos;
        vec3 color;
      };
      explicit DebugDrawer();
      ~DebugDrawer() {
        if (vao) {
          glDeleteVertexArrays(1, &vao);
          vao = 0;
        }
      }

      std::vector<DebugVertex> debug_vertices;
      vertex_buffer_dynamic vbo;
      Shader shader;
      GLuint vao{};

      static constexpr vec3 obb_local_corners[8] = {
        {-1, -1, -1},
        { 1, -1, -1},
        { 1,  1, -1},
        {-1,  1, -1},
        {-1, -1,  1},
        { 1, -1,  1},
        { 1,  1,  1},
        {-1,  1,  1},
      };

      static constexpr int obb_edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0},
        {4,5},{5,6},{6,7},{7,4},
        {0,4},{1,5},{2,6},{3,7}
      };
  };
}
