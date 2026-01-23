#pragma once
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/active_camera.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "game/ecs/components/transform.hpp"
#include "game/ecs/components/render_target.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/renderer/vertex_buffer.hpp"
#include "graphics/renderer/framebuffer_manager.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

void chunk_renderer_system(ECS& ecs, const ChunkManager& cm, Camera* cam, FrustumVolume& wanted_fv, FramebufferManager& fb)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	const Shader& shader = cm.getShader();
	ecs.for_each_components<Transform, RenderTarget, ActiveCamera>(
	    [&](Entity e, Transform& ts, RenderTarget& rt, ActiveCamera) {
		auto& cur_fb = fb.get(e);
		cur_fb.bind_draw();
		glViewport(0, 0, cur_fb.width(), cur_fb.height());
		glClear(DEPTH_TEST ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT : GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		for(auto* chunk : cm.getOpaqueChunks(wanted_fv))
			chunk->renderOpaqueMesh(shader, cm.getVAO());


		//glDisable(GL_CULL_FACE);

		//for(auto& chunk : cm.getTransparentChunks(wanted_fv, ts))
			//chunk->renderTransparentMesh(shader);

		//if (FACE_CULLING)
			//glEnable(GL_CULL_FACE);

	    });
}
