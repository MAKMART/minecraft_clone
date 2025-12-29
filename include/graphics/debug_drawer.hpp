#pragma once
#include "graphics/renderer/vertex_buffer.hpp"
#include "core/aabb.hpp"
#include <vector>
#include "shader.hpp"

class DebugDrawer
{
      public:
	DebugDrawer();
	~DebugDrawer();

	void addAABB(const AABB& box, const glm::vec3& color);
	void addOBB(const glm::mat4& transform, const glm::vec3& halfExtents, const glm::vec3& color);
	void addRay(glm::vec3 start, glm::vec3 dir, glm::vec3 color);
	void draw(const glm::mat4& viewProj);

      private:
	struct DebugAABB {
		AABB      box;
		glm::vec3 color;
		DebugAABB(const AABB& b, const glm::vec3& c) : box(b), color(c)
		{
		}
	};

	struct DebugOBB {
		glm::mat4 transform;
		glm::vec3 halfExtents;
		glm::vec3 color;
		DebugOBB(const glm::mat4& t, const glm::vec3& h, const glm::vec3& c)
		    : transform(t), halfExtents(h), color(c)
		{
		}
	};
	std::vector<DebugAABB> aabbs;
	std::vector<DebugOBB>  obbs;
	std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3>> rays;
	std::vector<glm::vec3> vertices;
	Shader*                shader;
	GLuint                 vao = 0;
	GLuint                 vao_lines = 0;
	VB vbo;
	VB vbo2;

	void initGLResources();
	void checkGLError(const std::string& operation);
	float line_vertices[6] = {
		0.0f, 0.0f, 0.0f, // start
		0.0f, 1.0f, 0.0f  // end along +Y
	};
};
