module;
#include <gl.h>
module debug_drawer;

import gl_state;
import logger;

void DebugDrawer::add_aabb(const AABB& b, glm::vec3 color)
{
  glm::vec3 min = b.min;
  glm::vec3 max = b.max;

  auto add_line = [&](glm::vec3 a, glm::vec3 c)
  {
    debug_vertices.push_back({a, color });
    debug_vertices.push_back({c, color });
  };

  // bottom square
  add_line({min.x, min.y, min.z}, {max.x, min.y, min.z});
  add_line({max.x, min.y, min.z}, {max.x, min.y, max.z});
  add_line({max.x, min.y, max.z}, {min.x, min.y, max.z});
  add_line({min.x, min.y, max.z}, {min.x, min.y, min.z});

  // top square
  add_line({min.x, max.y, min.z}, {max.x, max.y, min.z});
  add_line({max.x, max.y, min.z}, {max.x, max.y, max.z});
  add_line({max.x, max.y, max.z}, {min.x, max.y, max.z});
  add_line({min.x, max.y, max.z}, {min.x, max.y, min.z});

  // verticals
  add_line({min.x, min.y, min.z}, {min.x, max.y, min.z});
  add_line({max.x, min.y, min.z}, {max.x, max.y, min.z});
  add_line({max.x, min.y, max.z}, {max.x, max.y, max.z});
  add_line({min.x, min.y, max.z}, {min.x, max.y, max.z});
}
void DebugDrawer::add_obb(const glm::mat4& transform, glm::vec3 half_extents, glm::vec3 color)
{
  glm::mat4 scale = glm::scale(glm::mat4(1.0f), half_extents);

  glm::mat4 model = transform * scale;

  glm::vec3 world_corners[8];

  for (int i = 0; i < 8; i++) {
    glm::vec4 p = model * glm::vec4(obb_local_corners[i], 1.0f);
    world_corners[i] = glm::vec3(p);
  }

  for (auto& e : obb_edges) {
    debug_vertices.push_back({ world_corners[e[0]], color });
    debug_vertices.push_back({ world_corners[e[1]], color });
  }
}
void DebugDrawer::add_ray(glm::vec3 origin, glm::vec3 d, glm::vec3 color)
{
  glm::vec3 e = origin + d * 10000.0f;

  debug_vertices.push_back({origin, color });
  debug_vertices.push_back({e, color });
}
void DebugDrawer::draw(const glm::mat4& view_projection)
{
  vbo.update( debug_vertices.data(), debug_vertices.size() * sizeof(DebugVertex));
	shader.setMat4("viewProj", view_projection);
  shader.use();
	glBindVertexArray(vao);
  DrawArraysWrapper(GL_LINES, 0, debug_vertices.size());
	checkGLError("AABBDebugDrawer::draw - glBindVertexArray(0)");
  debug_vertices.clear();
}
void DebugDrawer::checkGLError(const std::string& operation)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		log::system_error("AABBDebugDrawer", "{} failed: OpenGL error {}", operation, err);
	}
}
