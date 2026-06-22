module;
#include <glad/gl.h>
export module engine.renderer:gl_backend.gl_state;

import engine.core;
import engine.math;

using namespace engine::math;
export struct GLState {
  public:
    GLState() = delete;

    void clear_depth() noexcept;
    // void apply_state(const RenderState& target);

    void set_line_width(f32 width);
    void set_wireframe(bool enable);
    void set_depth_test(bool enable);
    void set_depth_func(GLenum func);
    void set_depth_write(bool enable);
    void set_blending(bool enable);
    void set_face_culling(bool enable);
    void set_msaa(bool enable);
    void set_blend_func(GLenum src, GLenum dst);
    void set_scissor_test(bool enable);
    void set_stencil_test(bool enable);
    void set_scissor(i32 x, i32 y, u32 width, u32 height);
    void set_clear_color(const vec4& color);
    void set_color_mask(bool r, bool g, bool b, bool a);
    void set_viewport(i32 x, i32 y, u32 width, u32 height);

    bool is_wireframe() noexcept { return m_wireframe_mode; }
    f32 get_line_width() noexcept { return m_line_width; }
    bool is_depth_test() noexcept { return m_depth_test; }
    GLenum get_depth_func() noexcept { return m_depth_func; }
    bool is_blending() noexcept { return m_blending; }
    bool is_face_culling() noexcept { return m_face_culling; }
    bool is_msaa_enabled() noexcept { return m_msaa; }
    struct BlendFactors { GLenum src; GLenum dst; };
    BlendFactors get_blend_func() noexcept { return { m_src_blend, m_dst_blend }; }
    bool is_scissor_test() noexcept { return m_scissor_test; };
    bool is_stencil_test() noexcept { return m_stencil_test; };
    const ivec4 get_scissor_rect() noexcept { return m_scissor_rect; };
    const vec4 get_clear_color() noexcept { return m_clear_color; };

    void init_capabilities() noexcept;
    void sync() noexcept;

  private:
    // all of these should be so that the user can just access them or maybe private so that the users can't directly change them, to mimic opengl global state
    f32  m_line_width{1.4f};
    bool m_wireframe_mode{false};
    bool m_depth_test{true};
    GLenum m_depth_func{GL_GREATER};
    bool m_blending{true};
    bool m_face_culling{true};
    bool m_msaa{false};
    GLenum m_src_blend{GL_SRC_ALPHA};
    GLenum m_dst_blend{GL_ONE_MINUS_SRC_ALPHA};
    bool m_scissor_test{false};
    bool m_stencil_test{false};
    // Using glm::ivec4 for easy rect storage (x, y, w, h)
    ivec4 m_scissor_rect{0};
    ivec4 m_viewport{0};
    vec4 m_clear_color{0.1f, 0.1f, 0.1f, 1.0f};

    // Hardware/Context capabilities (Set once at init)
    u32 m_depth_bits{};
    u32 m_stencil_bits{};
    u32 m_msaa_strength{};

    bool m_initialized{false};
};


using namespace engine::core;
using namespace engine::math;

void GLState::clear_depth() noexcept {
  if (m_depth_func == GL_GREATER) {
    glClearDepth(0.0);
  } else if (m_depth_func == GL_LESS) {
    glClearDepth(1.0);
  }
}
/*
void GLState::apply_state(const RenserState& target) {
  set_depth_test(target.depth_test);
  set_depth_write(target.depth_write);
  set_depth_func(target.depth_func);

  set_blending(target.blending);
  if (target.blending) {
    set_blend_func(target.src_blend, target.dst_blend);
  }

  set_face_culling(target.face_culling);
  set_wireframe(target.wireframe);
  set_line_width(target.line_width);
  set_color_mask(target.color_mask_r, target.color_mask_g, target.color_mask_b, target.color_mask_a);
}
*/
void GLState::set_line_width(f32 width) {
    if (m_line_width != width) {
        glLineWidth(width);
        m_line_width = width;
    }
}

void GLState::set_wireframe(bool enable) {
    if (m_wireframe_mode != enable) {
        glPolygonMode(GL_FRONT_AND_BACK, enable ? GL_LINE : GL_FILL);
        m_wireframe_mode = enable;
    }
}

void GLState::set_depth_test(bool enable) {
    if (m_depth_test != enable) {
        enable ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        m_depth_test = enable;
    }
}

void GLState::set_depth_func(GLenum func) {
	if (m_depth_func == func)
		return;

	m_depth_func = func;

	if (m_depth_test)
		glDepthFunc(func);
  clear_depth();
}
void GLState::set_depth_write(bool enable) {
  static bool cached_depth_write = true; // Add to private members if preferred
  if (cached_depth_write != enable) {
    glDepthMask(enable ? GL_TRUE : GL_FALSE);
    cached_depth_write = enable;
  }
}
void GLState::set_blending(bool enable) {
	if (m_blending != enable) {
		enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
		m_blending = enable;
	}
}
void GLState::set_face_culling(bool enable) {
	if (m_face_culling != enable) {
		enable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
		m_face_culling = enable;
	}
}
void GLState::set_msaa(bool enable) {
	if (m_msaa != enable) {
		enable ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
		m_msaa = enable;
	}
}

void GLState::set_blend_func(GLenum src, GLenum dst) {
    if (m_src_blend != src || m_dst_blend != dst) {
        glBlendFunc(src, dst);
        m_src_blend = src;
        m_dst_blend = dst;
    }
}

void GLState::set_scissor_test(bool enable) {
    if (m_scissor_test != enable) {
        enable ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
        m_scissor_test = enable;
    }
}

void GLState::set_stencil_test(bool enable) {
    if (m_stencil_test != enable) {
        enable ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
        m_stencil_test = enable;
    }
}

void GLState::set_scissor(i32 x, i32 y, u32 width, u32 height) {
    set_scissor_test(true); // Scissor only works if test is enabled
    if (m_scissor_rect.x != x ||
        m_scissor_rect.y != y || 
        static_cast<u32>(m_scissor_rect.z) != width ||
        static_cast<u32>(m_scissor_rect.w) != height) {
        glScissor(x, y, width, height);
        m_scissor_rect = {x, y, width, height};
    }
}

void GLState::set_clear_color(const vec4& color) {
    if (m_clear_color != color) {
        glClearColor(color.r, color.g, color.b, color.a);
        m_clear_color = color;
    }
}
void GLState::set_color_mask(bool r, bool g, bool b, bool a) {
  // Note: bvec4 works well here for comparison
  static bvec4 cached_mask{true};
  if (cached_mask.r != r || cached_mask.g != g || cached_mask.b != b || cached_mask.a != a) {
    glColorMask(r, g, b, a);
    cached_mask = {r, g, b, a};
  }
}
void GLState::set_viewport(i32 x, i32 y, u32 width, u32 height) {
    ivec4 new_viewport{x, y, static_cast<i32>(width), static_cast<i32>(height)};
    if (m_viewport != new_viewport) {
        glViewport(x, y, width, height);
        m_viewport = new_viewport;
    }
}

void GLState::init_capabilities() noexcept {
  if (m_initialized) return;
  m_initialized = true;
  // Query actual bits allocated by the context
  GLint depth = 0, stencil = 0, samples = 0;
  glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_DEPTH, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &depth);
  glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE, &stencil);
  glGetIntegerv(GL_SAMPLES, &samples);

  m_depth_bits = static_cast<u32>(depth);
  m_stencil_bits = static_cast<u32>(stencil);
  m_msaa_strength = static_cast<u32>(samples);

  // Check for specific 4.6 features
  GLint max_draw_buffers;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);

  //std::printf("OpenGL Context: Depth(%d), Stencil(%d), MSAA(%dx)\n",
  //m_depth_bits, m_stencil_bits, m_msaa_strength);
  logger::info("GL context: depth = {}, stencil = {}, msaa = {}", m_depth_bits, m_stencil_bits, m_msaa_strength);
}

void GLState::sync() noexcept {
    // Fixed logic for Cull Face
    m_face_culling ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
    glCullFace(GL_BACK); // Standard default
    glFrontFace(GL_CCW); // Essential for consistent voxel face rendering

    glLineWidth(m_line_width);
    glPolygonMode(GL_FRONT_AND_BACK, m_wireframe_mode ? GL_LINE : GL_FILL);

    m_depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
    glDepthFunc(m_depth_func);
    clear_depth();

    m_blending ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    glBlendFunc(m_src_blend, m_dst_blend);

    m_msaa ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);

    m_scissor_test ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
    if (m_scissor_test)
      glScissor(m_scissor_rect.x, m_scissor_rect.y, m_scissor_rect.z, m_scissor_rect.w);

    glViewport(m_viewport.x, m_viewport.y, m_viewport.z, m_viewport.w);
    glClearColor(m_clear_color.r, m_clear_color.g, m_clear_color.b, m_clear_color.a);

    // Modern 4.6 feature: Seamless cubemaps for skyboxes
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}
