#include "game/ecs/systems/chunk_renderer_system.hpp"
#include "chunk/chunk_manager.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/active_camera.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/render_target.hpp"
#include "graphics/renderer/vertex_buffer.hpp"
#include "graphics/renderer/framebuffer_manager.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

void chunk_renderer_system(ECS& ecs, ChunkManager& cm, Camera* cam, FrustumVolume& wanted_fv, FramebufferManager& fb)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	ecs.for_each_components<Transform, RenderTarget, ActiveCamera>(
	    [&](Entity e, Transform& ts, RenderTarget& rt, ActiveCamera) {
		const auto& cur_fb = fb.get(e);
		cur_fb.bind_draw();
		glViewport(0, 0, cur_fb.width(), cur_fb.height());
		glClear(DEPTH_TEST ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		cm.render_opaque(ts, wanted_fv);

		//glDisable(GL_CULL_FACE);

		//cm.render_transparent(ts, wanted_fv);

		//if (FACE_CULLING)
			//glEnable(GL_CULL_FACE);

	    });
}
