#pragma once
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/active_camera.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "game/ecs/components/transform.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

void chunk_renderer_system(ECS& ecs, const ChunkManager& cm, const Shader& shader, float time, Texture& Atlas)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	ecs.for_each_components<Camera, ActiveCamera, Transform, FrustumVolume>(
	    [&](Entity e, Camera& cam, ActiveCamera& ac, Transform& ts, FrustumVolume& fv) {
		shader.use();
		shader.setMat4("projection", cam.projectionMatrix);
		shader.setMat4("view", cam.viewMatrix);
		shader.setFloat("time", time);

		Atlas.Bind(0);
		glBindVertexArray(cm.getVAO());

		for(auto& chunk : cm.getOpaqueChunks(fv)) {
			chunk->renderOpaqueMesh(shader);
		};
		glDisable(GL_CULL_FACE);
		if (BLENDING) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		for(auto& chunk : cm.getTransparentChunks(fv, ts)) {
			chunk->renderTransparentMesh(shader);
		};


		if (FACE_CULLING) {
			glEnable(GL_CULL_FACE);
		}

		glBindVertexArray(0);
		Atlas.Unbind(0);
	    });
}
