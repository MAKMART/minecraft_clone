module;
#include <glad/glad.h>
export module gl_state;
import glm;
export struct GLState {

	public:
		GLState() = delete;

		static void set_line_width(float width);
		static void set_wireframe(bool enable);
		static void set_depth_test(bool enable);
		static void set_depth_func(GLenum func);
		static void set_blending(bool enable);
		static void set_face_culling(bool enable);
		static void set_msaa(bool enable);
		static void set_blend_func(GLenum src, GLenum dst);
		static void set_scissor_test(bool enable);
		static void set_stencil_test(bool enable);
		static void set_scissor(int x, int y, int width, int height);
		static void set_clear_color(const glm::vec4& color);
		static void set_viewport(int x, int y, int width, int height);

		static bool is_wireframe() noexcept { return m_wireframe_mode; }
		static float get_line_width() noexcept { return m_line_width; }
		static bool is_depth_test() noexcept { return m_depth_test; }
		static GLenum get_depth_func() noexcept { return m_depth_func; }
		static bool is_blending() noexcept { return m_blending; }
		static bool is_face_culling() noexcept { return m_face_culling; }
		static bool is_msaa_enabled() noexcept { return m_msaa; }
		struct BlendFactors { GLenum src; GLenum dst; };
		static BlendFactors get_blend_func() noexcept { return { m_src_blend, m_dst_blend }; }
		static bool is_scissor_test() noexcept { return m_scissor_test; };
		static bool is_stencil_test() noexcept { return m_stencil_test; };
		static const glm::ivec4 get_scissor_rect() noexcept { return m_scissor_rect; };
		static const glm::vec4 get_clear_color() noexcept { return m_clear_color; };

		static void init_capabilities() noexcept;
		static void sync() noexcept;

	private:
		// all of these should be static so that the user can just access them or maybe private so that the users can't directly change them, to mimic opengl global state
		static float  m_line_width;
		static bool m_wireframe_mode;
		static bool m_depth_test;
		static GLenum m_depth_func;
		static bool m_blending;
		static bool m_face_culling;
		static bool m_msaa;
		static GLenum m_src_blend;
		static GLenum m_dst_blend;
		static bool m_scissor_test;
		static bool m_stencil_test;
		// Using glm::ivec4 for easy rect storage (x, y, w, h)
		static glm::ivec4 m_scissor_rect;
		static glm::ivec4 m_viewport;
		static glm::vec4 m_clear_color;

		// Hardware/Context capabilities (Set once at init)
		static inline int m_depth_bits = 0;
		static inline int m_stencil_bits = 0;
		static inline int m_msaa_strength = 0;

		static inline bool m_initialized = false;
};
