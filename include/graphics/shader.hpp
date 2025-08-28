#pragma once
#include <GL/glew.h>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include "core/logger.hpp"

class Shader
{
      private:
	unsigned int                                   ID;
	std::string                                    name;
	std::filesystem::path                          vertexShaderPath;
	std::filesystem::path                          fragmentShaderPath;
	mutable std::unordered_map<std::string, GLint> uniformCache;
	mutable std::mutex                             uniformCacheMutex;

	std::filesystem::file_time_type lastVertexWriteTime;
	std::filesystem::file_time_type lastFragmentWriteTime;

      public:
	unsigned int getProgramID() const
	{
		return ID;
	}

	std::string getName() const
	{
		return name;
	}

	Shader(const std::string& _name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	    : name(_name), vertexShaderPath(vertexPath), fragmentShaderPath(fragmentPath)
	{
		ID = loadAndCompile(vertexPath, fragmentPath);
		reflectUniforms();
		lastVertexWriteTime   = std::filesystem::last_write_time(vertexShaderPath);
		lastFragmentWriteTime = std::filesystem::last_write_time(fragmentShaderPath);
	}

	Shader(std::string _name, std::string vertexPath, std::string fragmentPath)
	    : name(_name), vertexShaderPath(vertexPath), fragmentShaderPath(fragmentPath)
	{
		ID = loadAndCompile(vertexShaderPath, fragmentShaderPath);
		reflectUniforms();
		lastVertexWriteTime   = std::filesystem::last_write_time(vertexShaderPath);
		lastFragmentWriteTime = std::filesystem::last_write_time(fragmentShaderPath);
	}

	Shader(const Shader&)            = delete;
	Shader& operator=(const Shader&) = delete;

	~Shader()
	{
		if (glIsProgram(ID)) {
			glDeleteProgram(ID);
		}
	}

	void use() const
	{
		if (glIsProgram(ID)) {
			glUseProgram(ID);
		} else {
			log::system_error("Shader", "[{}] Program is not valid! ID = {}", name, ID);
		}
	}

	bool checkAndReloadIfModified()
	{
		try {
			auto currentVertexTime   = std::filesystem::last_write_time(vertexShaderPath);
			auto currentFragmentTime = std::filesystem::last_write_time(fragmentShaderPath);

			if (currentVertexTime != lastVertexWriteTime || currentFragmentTime != lastFragmentWriteTime) {
				lastVertexWriteTime   = currentVertexTime;
				lastFragmentWriteTime = currentFragmentTime;
				return reload();
			}
		} catch (const std::exception& e) {
			log::system_warn("Shader", "[{}] Error checking shader file timestamps: {}", name, e.what());
		}
		return false;
	}

	bool reload()
	{
		try {
			unsigned int newProgram = loadAndCompile(vertexShaderPath, fragmentShaderPath);

			if (glIsProgram(ID)) {
				glDeleteProgram(ID);
			}

			ID = newProgram;
			uniformCache.clear(); // Uniforms may have changed
			reflectUniforms();

			log::system_info("Shader", "[{}] Reloaded shader from {} and {}", name, vertexShaderPath.string(), fragmentShaderPath.string());
			return true;
		} catch (const std::exception& e) {
			log::system_error("Shader", "[{}] Failed to reload shader: {}", name, e.what());
			return false;
		}
	}

	void setBool(const std::string& name, bool value) const
	{
		GLint location = getUniformLocation(name);
		if (location != -1) {
			glProgramUniform1i(ID, location, static_cast<int>(value));
		}
	}

	void setInt(const std::string& name, int value) const
	{
		GLint location = getUniformLocation(name);
		if (location != -1) {
			glProgramUniform1i(ID, location, value);
		}
	}

	void setFloat(const std::string& name, float value) const
	{
		GLint location = getUniformLocation(name);
		if (location != -1) {
			glProgramUniform1f(ID, location, value);
		}
	}

	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		GLint location = getUniformLocation(name);
		if (location != -1) {
			glProgramUniform2fv(ID, location, 1, glm::value_ptr(value));
		}
	}

	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		GLint location = getUniformLocation(name);
		if (location != -1) {
			glProgramUniform3fv(ID, location, 1, glm::value_ptr(value));
		}
	}

	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		GLint location = getUniformLocation(name);
		if (location != -1) {
			glProgramUniform4fv(ID, location, 1, glm::value_ptr(value));
		}
	}

	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		GLint location = getUniformLocation(name);
		if (location != -1) {
			glProgramUniformMatrix4fv(ID, location, 1, GL_FALSE, glm::value_ptr(mat));
		}
	}

      private:
	void reflectUniforms()
	{

		std::lock_guard<std::mutex> lock(uniformCacheMutex);
		uniformCache.clear();

		GLint uniformCount = 0;
		glGetProgramiv(ID, GL_ACTIVE_UNIFORMS, &uniformCount);

		char nameBuffer[256];
		for (GLint i = 0; i < uniformCount; ++i) {
			GLsizei length;
			GLint   size;
			GLenum  type;

			glGetActiveUniform(ID, i, sizeof(nameBuffer), &length, &size, &type, nameBuffer);
			std::string name(nameBuffer, length);

			GLint location = glGetUniformLocation(ID, name.c_str());
			if (location != -1) {
				uniformCache[name] = location;
			}
		}

		log::system_info("Shader", "[{}] Reflected and cached {} active uniforms.", name, uniformCache.size());
	}

	GLint getUniformLocation(const std::string& uni_name) const
	{
		std::lock_guard<std::mutex> lock(uniformCacheMutex);
		auto                        it = uniformCache.find(uni_name);
		if (it != uniformCache.end()) {
			return it->second;
		}
		GLint location = glGetUniformLocation(ID, name.c_str());
		if (location == -1) {
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", name, uni_name);
		} else {
			uniformCache[uni_name] = location;
		}
		return location;
	}

	unsigned int loadAndCompile(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	{
		std::string vertexCode;
		std::string fragmentCode;

		std::ifstream vShaderFile;
		std::ifstream fShaderFile;
		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);
			std::stringstream vShaderStream, fShaderStream;
			vShaderStream << vShaderFile.rdbuf();
			fShaderStream << fShaderFile.rdbuf();
			vShaderFile.close();
			fShaderFile.close();
			vertexCode   = vShaderStream.str();
			fragmentCode = fShaderStream.str();

			if (vertexCode.empty() || fragmentCode.empty()) {
				log::system_error("Shader", "[{}] Shader source code is empty.", name);
			}
		} catch (std::ifstream::failure& e) {
			log::system_error("Shader", "[{}] Failed to read shader files: {}", name, std::string(e.what()));
		}

		const GLchar* vShaderCode = vertexCode.c_str();
		const GLchar* fShaderCode = fragmentCode.c_str();

		unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, nullptr);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");

		unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, nullptr);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");

		unsigned int program = glCreateProgram();
		glAttachShader(program, vertex);
		glAttachShader(program, fragment);
		glLinkProgram(program);
		checkCompileErrors(program, "PROGRAM");

		glValidateProgram(program);
		GLint programSuccess;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &programSuccess);
		if (!programSuccess) {
			char infoLog[1024];
			glGetProgramInfoLog(program, 1024, nullptr, infoLog);
			log::system_error("Shader", "[{}] Shader validation failed: {}", name, infoLog);
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);
		return program;
	}

	void checkCompileErrors(unsigned int shader, const std::string& type)
	{
		int  success;
		char infoLog[1024];

		if (type != "PROGRAM") {
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success) {
				glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
				log::system_error("Shader", "[{}] SHADER_COMPILATION_ERROR of type: {}\n{}", name, type, infoLog);
			}
		} else {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
				log::system_error("Shader", "[{}] PROGRAM_LINKING_ERROR of type: {}\n{}", name, type, infoLog);
			}
		}
	}
};
