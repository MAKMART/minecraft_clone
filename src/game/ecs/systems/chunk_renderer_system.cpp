module;
#include <glad/glad.h>
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif
module chunk_renderer_system;

import core;
import gl_state;
import glm;
import std;

void chunk_renderer_system(ECS& ecs, ChunkManager& cm, Camera* cam, FrustumVolume& wanted_fv, FramebufferManager& fb)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	ecs.for_each_components<Transform, RenderTarget, ActiveCamera>(
	    [&](Entity e, Transform& ts, RenderTarget& rt, ActiveCamera) {
		const auto& cur_fb = fb.get(e);
		cur_fb.bind_draw();
		GLState::set_viewport(0, 0, cur_fb.width(), cur_fb.height());
		// GLState::get_depth_test() ensures we only clear DEPTH if it's active
		GLbitfield clear_mask = GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
		if (GLState::is_depth_test()) {
		clear_mask |= GL_DEPTH_BUFFER_BIT;
		}
		glClear(clear_mask);

		GLState::set_face_culling(true);
		GLState::set_blending(false);
		cm.render_opaque(ts, wanted_fv);

		//GLState::set_face_culling(false); // Water usually needs both sides or special culling
		//GLState::set_blending(true);
		//GLState::set_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//cm.render_transparent(ts, wanted_fv);
	    });
}
