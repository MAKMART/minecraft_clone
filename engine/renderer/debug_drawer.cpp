module;
#include <glad/gl.h>
module engine.renderer;

// import gl_state;
// import logger;
import engine.math;

namespace engine::renderer {
  DebugDrawer::DebugDrawer()
    : shader("Debug", std::filesystem::path("debug_vert.glsl"), std::filesystem::path("debug_frag.glsl")),
    vbo(vertex_buffer_dynamic(10000)) {
    glGenVertexArrays(1, &vao);

    glVertexArrayVertexBuffer(vao, 0, vbo.id(), 0, sizeof(DebugVertex));
    static constexpr std::size_t pos_offset   = 0;
    static constexpr std::size_t color_offset = sizeof(vec3);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, pos_offset);
    glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, color_offset);

    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);

    glEnableVertexArrayAttrib(vao, 0);
    glEnableVertexArrayAttrib(vao, 1);
  }
  void DebugDrawer::add_aabb(const AABB& b, vec3 color) {
    vec3 min = b.min;
    vec3 max = b.max;

    auto add_line = [&](vec3 a, vec3 c) {
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
  void DebugDrawer::add_obb(const mat4& transform, vec3 half_extents, vec3 color) {
    mat4 scale = engine::math::scale(mat4(1.0f), half_extents);

    mat4 model = transform * scale;

    vec3 world_corners[8];

    for (std::size_t i = 0; i < 8; i++) {
      vec4 p = model * vec4(obb_local_corners[i], 1.0f);
      world_corners[i] = vec3(p);
    }

    for (auto& e : obb_edges) {
      debug_vertices.push_back({ world_corners[e[0]], color });
      debug_vertices.push_back({ world_corners[e[1]], color });
    }
  }
  void DebugDrawer::add_ray(vec3 origin, vec3 d, vec3 color) {
    vec3 e = origin + d * 10000.0f;

    debug_vertices.push_back({origin, color });
    debug_vertices.push_back({e, color });
  }
  void DebugDrawer::draw(const mat4& view_projection) {
    vbo.update(debug_vertices.data(), debug_vertices.size() * sizeof(DebugVertex));
    shader.set_mat4("viewProj", view_projection);
    shader.use();
    glBindVertexArray(vao);
    // DrawArraysWrapper(GL_LINES, 0, debug_vertices.size());
    glDrawArrays(GL_LINES, 0, debug_vertices.size());
    debug_vertices.clear();
  }
}
