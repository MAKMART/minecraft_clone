#include "graphics/debug_drawer.hpp"
#include <glad/glad.h>
#include "core/defines.hpp"
#include "core/logger.hpp"

DebugDrawer::DebugDrawer()
{
	// TODO: Adjust paths
	shader = new Shader("Debug", std::filesystem::path(SHADERS_DIRECTORY / "debug_vert.glsl"), std::filesystem::path(SHADERS_DIRECTORY / "debug_frag.glsl"));

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
	if (vao != 0) {
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
	if (vao_lines != 0) {
		glDeleteVertexArrays(1, &vao_lines);
		vao_lines = 0;
	}
	delete shader;
}

void DebugDrawer::addAABB(const AABB& box, const glm::vec3& color)
{
	aabbs.emplace_back(box, color); // Use emplace_back
}

void DebugDrawer::addRay(glm::vec3 start, glm::vec3 dir, glm::vec3 color) {
	rays.emplace_back(std::tuple<glm::vec3, glm::vec3, glm::vec3>(start, dir, color));
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

	glDisable(GL_DEPTH_TEST);
	for (auto& ray_tuple : rays) {
		auto& [origin, direction, color] = ray_tuple;
		shader->setVec3("debugColor", color);

		direction = glm::normalize(direction);
		glm::vec3 up = glm::vec3(0, 1, 0); // unit line in +Y
		glm::vec3 axis = glm::cross(up, direction);
		float angle = acos(glm::clamp(glm::dot(up, glm::normalize(direction)), -1.0f, 1.0f));

		glm::mat4 rotation = glm::mat4(1.0f);
		if (glm::length(axis) > 0.0001f) {
			rotation = glm::rotate(glm::mat4(1.0f), angle, glm::normalize(axis));
		}

		float length = glm::length(direction) > 0.0001f ? glm::length(direction) : 10000.0f;

		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, length, 1.0f));
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), origin);

		glm::mat4 model = translation * rotation * scale;

		shader->setMat4("model", model);
		glBindVertexArray(vao_lines);
		glDrawArrays(GL_LINES, 0, 2);
	}

	glEnable(GL_DEPTH_TEST);

	glBindVertexArray(0);
	checkGLError("AABBDebugDrawer::draw - glBindVertexArray(0)");

	rays.clear();
	aabbs.clear();
	obbs.clear();
}

void DebugDrawer::initGLResources()
{
	glGenVertexArrays(1, &vao);
	glGenVertexArrays(1, &vao_lines);
	vbo = VB(vertices.data(), vertices.size() * sizeof(glm::vec3), VB::usage::static_draw);
	vbo2 = VB(line_vertices, sizeof(line_vertices), VB::usage::static_draw);

	glBindVertexArray(vao);
	glVertexArrayVertexBuffer(vao, 0, vbo.id(), 0, sizeof(glm::vec3));
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao, 0);
	glBindVertexArray(0);

	glBindVertexArray(vao_lines);
	glVertexArrayVertexBuffer(vao_lines, 0, vbo2.id(), 0, sizeof(glm::vec3));
	glVertexArrayAttribFormat(vao_lines, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glEnableVertexArrayAttrib(vao_lines, 0);
	checkGLError("AABBDebugDrawer::initGLResources");
}

void DebugDrawer::checkGLError(const std::string& operation)
{
	GLenum err = glGetError();
	if (err != GL_NO_ERROR) {
		log::system_error("AABBDebugDrawer", "{} failed: OpenGL error {}", operation, err);
	}
}
