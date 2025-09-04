#include "graphics/debug_drawer.hpp"
#include <glad/glad.h>
#include "core/logger.hpp"

DebugDrawer::DebugDrawer()
{
	// TODO: Adjust paths
	shader = new Shader("Debug", std::filesystem::path("shaders/debug_vert.glsl"), std::filesystem::path("shaders/debug_frag.glsl"));

	vertices = {
	    // Front face
	    {-0.5f, -0.5f, 0.5f},
	    {0.5f, -0.5f, 0.5f},
	    {0.5f, -0.5f, 0.5f},
	    {0.5f, 0.5f, 0.5f},
	    {0.5f, 0.5f, 0.5f},
	    {-0.5f, 0.5f, 0.5f},
	    {-0.5f, 0.5f, 0.5f},
	    {-0.5f, -0.5f, 0.5f},
	    // Back face
	    {-0.5f, -0.5f, -0.5f},
	    {0.5f, -0.5f, -0.5f},
	    {0.5f, -0.5f, -0.5f},
	    {0.5f, 0.5f, -0.5f},
	    {0.5f, 0.5f, -0.5f},
	    {-0.5f, 0.5f, -0.5f},
	    {-0.5f, 0.5f, -0.5f},
	    {-0.5f, -0.5f, -0.5f},
	    // Connecting edges
	    {-0.5f, -0.5f, 0.5f},
	    {-0.5f, -0.5f, -0.5f},
	    {0.5f, -0.5f, 0.5f},
	    {0.5f, -0.5f, -0.5f},
	    {0.5f, 0.5f, 0.5f},
	    {0.5f, 0.5f, -0.5f},
	    {-0.5f, 0.5f, 0.5f},
	    {-0.5f, 0.5f, -0.5f}};

	initGLResources();
}

DebugDrawer::~DebugDrawer()
{
	destroyGLResources();
	delete shader;
}

void DebugDrawer::addAABB(const AABB& box, const glm::vec3& color)
{
	aabbs.emplace_back(box, color); // Use emplace_back
}

void DebugDrawer::addOBB(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec3& color)
{
	obbs.emplace_back(transform, halfExtents, color); // Use emplace_back
}

void DebugDrawer::draw(const glm::mat4& viewProj)
{
	if (aabbs.empty() && obbs.empty())
		return;

	shader->use();
	shader->setMat4("viewProj", viewProj);

	glBindVertexArray(vao);
	checkGLError("AABBDebugDrawer::draw - glBindVertexArray");

	for (const auto& aabb : aabbs) {
		glm::vec3 size   = aabb.box.max - aabb.box.min;
		glm::vec3 center = (aabb.box.min + aabb.box.max) * 0.5f;
		glm::mat4 model  = glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), size);
		shader->setMat4("model", model);
		shader->setVec3("debugColor", aabb.color);
		glDrawArrays(GL_LINES, 0, 24);
		checkGLError("AABBDebugDrawer::draw - AABB glDrawArrays");
	}

	for (const auto& obb : obbs) {
		glm::mat4 model = obb.transform * glm::scale(glm::mat4(1.0f), obb.halfExtents * 2.0f);
		shader->setMat4("model", model);
		shader->setVec3("debugColor", obb.color);
		glDrawArrays(GL_LINES, 0, 24);
		checkGLError("AABBDebugDrawer::draw - OBB glDrawArrays");
	}

	glBindVertexArray(0);
	checkGLError("AABBDebugDrawer::draw - glBindVertexArray(0)");

	aabbs.clear();
	obbs.clear();
}

void DebugDrawer::initGLResources()
{
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	checkGLError("AABBDebugDrawer::initGLResources");
}

void DebugDrawer::destroyGLResources()
{
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	if (vbo != 0) {
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
}

void DebugDrawer::checkGLError(const std::string& operation)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		log::system_error("AABBDebugDrawer", "{} failed: OpenGL error {}", operation, err);
	}
}
