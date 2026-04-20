module;
#include <gl.h>
export module debug_drawer;

import std;
import aabb;
import glm;
import shader;
import vertex_buffer;
import core;

export class DebugDrawer
{
	public:
		static DebugDrawer& get()
		{
			static DebugDrawer instance;
			return instance;
		}

		void add_aabb(const AABB& b, glm::vec3 color);
		void add_obb(const glm::mat4& transform, glm::vec3 half_extents, glm::vec3 color);
		void add_ray(glm::vec3 origin, glm::vec3 d, glm::vec3 color);
		void draw(const glm::mat4& view_projection);

	private:
		// Make these private because a singleton class should never have the possibility to be constructed/destructed because its lifetime is managed internally
    struct DebugVertex
    {
        glm::vec3 pos;
        glm::vec3 color;
    };
    explicit DebugDrawer() :
      shader("Debug", std::filesystem::path(SHADERS_DIRECTORY / "debug_vert.glsl"), std::filesystem::path(SHADERS_DIRECTORY / "debug_frag.glsl")),
      vbo(vertex_buffer_dynamic(10000))
    {
      glGenVertexArrays(1, &vao);

      glVertexArrayVertexBuffer(vao, 0, vbo.id(), 0, sizeof(DebugVertex));
      static constexpr std::size_t pos_offset   = 0;
      static constexpr std::size_t color_offset = sizeof(glm::vec3);
      glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, pos_offset);
      glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, color_offset);

      glVertexArrayAttribBinding(vao, 0, 0);
      glVertexArrayAttribBinding(vao, 1, 0);

      glEnableVertexArrayAttrib(vao, 0);
      glEnableVertexArrayAttrib(vao, 1);
      checkGLError("aabbdebugdrawer::aabbdebugdrawer");
    }
    ~DebugDrawer() {
      if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
      }
    }
    enum class DebugPrimitiveType : std::uint32_t
    {
      line = 0,
      tri  = 1,
    };

    std::vector<DebugVertex> debug_vertices;
		vertex_buffer_dynamic vbo;
    Shader shader;
		GLuint vao = 0;

		void checkGLError(const std::string& operation);
    static constexpr glm::vec3 obb_local_corners[8] = {
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
