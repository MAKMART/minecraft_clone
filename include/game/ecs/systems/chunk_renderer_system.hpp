#pragma once
#include "chunk/chunk.hpp"
#include "chunk/chunk_manager.hpp"
#include "game/ecs/ecs.hpp"
#include "game/ecs/components/active_camera.hpp"
#include "game/ecs/components/frustum_volume.hpp"
#include "game/ecs/components/transform.hpp"
#include "graphics/shader.hpp"
#include "graphics/texture.hpp"
#include "graphics/renderer/vertex_buffer.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

void chunk_renderer_system(ECS& ecs, const ChunkManager& cm, const Shader& test, float time, Texture& Atlas, GLuint VAO, Camera* cam, FrustumVolume& wanted_fv)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	const Shader& shader = cm.getShader();
	shader.use();
	shader.setMat4("projection", cam->projectionMatrix);
	shader.setMat4("view", cam->viewMatrix);
	shader.setFloat("time", time);
	test.use();
	test.setMat4("projection", cam->projectionMatrix);
	// Warning!! remove this after
	glm::mat4 temp_model = glm::mat4(1.0f);
	temp_model = glm::translate(temp_model, glm::vec3(-0.5f, 9.5f, 0.0f));
	temp_model = glm::scale(temp_model, glm::vec3(0.5f, 0.5f, 0.5f));
	test.setMat4("model", temp_model);
	test.setMat4("view", cam->viewMatrix);
	Atlas.Bind(0);
	float vertices[] = {
		// positions       // uvs
		-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
		3.0f, -1.0f, 0.0f,  2.0f, 0.0f,
		-1.0f,  3.0f, 0.0f,  0.0f, 2.0f
	};

	VB vb(vertices, sizeof(vertices));
	glVertexArrayVertexBuffer(
			VAO,
			0,                  // binding index
			vb.id(),
			0,                  // offset in buffer
			5 * sizeof(float)   // stride
			);
		glBindVertexArray(VAO);
	ecs.for_each_components<Transform, FrustumVolume>(
	    [&](Entity e, Transform& ts, FrustumVolume&) {

		test.use();
		if (BLENDING) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glBindVertexArray(0);

		shader.use();
		glBindVertexArray(cm.getVAO());
		for(auto& chunk : cm.getOpaqueChunks(wanted_fv)) {
			chunk->renderOpaqueMesh(shader);
		};
		glDisable(GL_CULL_FACE);
		for(auto& chunk : cm.getTransparentChunks(wanted_fv, ts)) {
			chunk->renderTransparentMesh(shader);
		};


		if (FACE_CULLING) {
			glEnable(GL_CULL_FACE);
		}
		glBindVertexArray(0);

	    });
		glBindVertexArray(0);
		Atlas.Unbind(0);
}
