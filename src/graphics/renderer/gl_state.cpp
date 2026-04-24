module;
#include <gl.h>
module gl_state;
import logger;

float GLState::m_line_width = 1.4f;
bool GLState::m_wireframe_mode = false;
bool GLState::m_depth_test     = true;
GLenum GLState::m_depth_func   = GL_GREATER; 
bool GLState::m_blending       = true;
bool GLState::m_face_culling   = true;
bool GLState::m_msaa           = false;
GLenum GLState::m_src_blend = GL_SRC_ALPHA;
GLenum GLState::m_dst_blend = GL_ONE_MINUS_SRC_ALPHA;
bool GLState::m_scissor_test = false;
bool GLState::m_stencil_test = false;
glm::ivec4 GLState::m_scissor_rect = {0, 0, 0, 0};
glm::ivec4 GLState::m_viewport = { 0, 0, 0, 0 };
glm::vec4  GLState::m_clear_color = { 0.1f, 0.1f, 0.1f, 1.0f };

void GLState::clear_depth() {
  glClearDepth(0.0f);
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
void GLState::set_line_width(float width) {
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

void GLState::set_scissor(int x, int y, int width, int height) {
    set_scissor_test(true); // Scissor only works if test is enabled
    if (m_scissor_rect.x != x || m_scissor_rect.y != y || 
        m_scissor_rect.z != width || m_scissor_rect.w != height) {
        glScissor(x, y, width, height);
        m_scissor_rect = {x, y, width, height};
    }
}

void GLState::set_clear_color(const glm::vec4& color) {
    if (m_clear_color != color) {
        glClearColor(color.r, color.g, color.b, color.a);
        m_clear_color = color;
    }
}
void GLState::set_color_mask(bool r, bool g, bool b, bool a) {
  // Note: glm::bvec4 works well here for comparison
  static glm::bvec4 cached_mask{true};
  if (cached_mask.r != r || cached_mask.g != g || cached_mask.b != b || cached_mask.a != a) {
    glColorMask(r, g, b, a);
    cached_mask = {r, g, b, a};
  }
}
void GLState::set_viewport(int x, int y, int width, int height) {
    glm::ivec4 new_viewport{ x, y, width, height };
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

	m_depth_bits = static_cast<int>(depth);
    m_stencil_bits = static_cast<int>(stencil);
    m_msaa_strength = static_cast<int>(samples);

    // Check for specific 4.6 features
    GLint max_draw_buffers;
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);

    //std::printf("OpenGL Context: Depth(%d), Stencil(%d), MSAA(%dx)\n",
                 //m_depth_bits, m_stencil_bits, m_msaa_strength);
	log::info("GL context: depth = {}, stencil = {}, msaa = {}", m_depth_bits, m_stencil_bits, m_msaa_strength);
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
