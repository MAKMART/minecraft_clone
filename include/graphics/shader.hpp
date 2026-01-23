#pragma once
#include <glad/glad.h>
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
	std::string                                    s_name;
	std::filesystem::path                          vertexShaderPath;
	std::filesystem::path                          fragmentShaderPath;
	std::filesystem::path                          computeShaderPath;
	mutable std::unordered_map<std::string, GLint> uniformCache;
	mutable std::mutex                             uniformCacheMutex;

	std::filesystem::file_time_type lastVertexWriteTime;
	std::filesystem::file_time_type lastFragmentWriteTime;
	std::filesystem::file_time_type lastComputeWriteTime;

	  public:
	unsigned int getProgramID() const { return ID; }
	std::string getName() const { return s_name; }

	Shader(const std::string& _name, const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	    : s_name(_name), vertexShaderPath(vertexPath), fragmentShaderPath(fragmentPath)
	{
		ID = loadAndCompile(vertexPath, fragmentPath);
		reflectUniforms();
		lastVertexWriteTime   = std::filesystem::last_write_time(vertexShaderPath);
		lastFragmentWriteTime = std::filesystem::last_write_time(fragmentShaderPath);
	}

	Shader(const std::string& _name, const std::filesystem::path& computePath)
        : s_name(_name), computeShaderPath(computePath)
    {
        ID = loadAndCompile(computePath, ShaderType::Compute);
        reflectUniforms();
        lastComputeWriteTime = std::filesystem::last_write_time(computeShaderPath);
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
			log::system_error("Shader", "[{}] Program is not valid! ID = {}", s_name, ID);
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
			log::system_warn("Shader", "[{}] Error checking shader file timestamps: {}", s_name, e.what());
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

			log::system_info("Shader", "[{}] Reloaded shader from {} and {}", s_name, vertexShaderPath.string(), fragmentShaderPath.string());
			return true;
		} catch (const std::exception& e) {
			log::system_error("Shader", "[{}] Failed to reload shader: {}", s_name, e.what());
			return false;
		}
	}

	void setBool(const std::string& name, bool value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform1i(ID, location, static_cast<int>(value));
	}

	void setInt(const std::string& name, int value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform1i(ID, location, value);
	}

	void setFloat(const std::string& name, float value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform1f(ID, location, value);
	}

	void setVec2(const std::string& name, const glm::vec2& value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform2fv(ID, location, 1, glm::value_ptr(value));
	}

	void setVec3(const std::string& name, const glm::vec3& value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform3fv(ID, location, 1, glm::value_ptr(value));
	}

	void setIVec3(const std::string& name, const glm::ivec3& value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform3iv(ID, location, 1, glm::value_ptr(value));
	}


	void setUVec3(const std::string& name, const glm::uvec3& value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform3uiv(ID, location, 1, glm::value_ptr(value));
	}


	void setVec4(const std::string& name, const glm::vec4& value) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}

		glProgramUniform4fv(ID, location, 1, glm::value_ptr(value));
	}

	void setMat4(const std::string& name, const glm::mat4& mat) const
	{
		if (ID == 0) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
			return;
		}

		GLint location = getUniformLocation(name);
		if (location == -1) {
#if defined(DEBUG)
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
			return;
		}
		glProgramUniformMatrix4fv(ID, location, 1, GL_FALSE, glm::value_ptr(mat));
	}

      private:
	enum class ShaderType { VertexFragment, Compute };
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
			std::string uni_name(nameBuffer, length);

			GLint location = glGetUniformLocation(ID, uni_name.c_str());
			if (location != -1) {
				uniformCache[uni_name] = location;
			}
		}
		if (!uniformCache.empty())
			log::system_info("Shader", "[{}] Reflected and cached {} active uniforms.", s_name, uniformCache.size());

	}

	GLint getUniformLocation(const std::string& uni_name) const
	{
		std::lock_guard<std::mutex> lock(uniformCacheMutex);
		auto                        it = uniformCache.find(uni_name);
		if (it != uniformCache.end()) {
			return it->second;
		}
		GLint location = glGetUniformLocation(ID, uni_name.c_str());
		if (location == -1) {
			log::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, uni_name);
		} else {
			uniformCache[uni_name] = location;
		}
		return location;
	}

	void readShaderFile(const std::filesystem::path& path, std::string& outCode)
    {
        std::ifstream shaderFile(path);
        if (!shaderFile.is_open())
            throw std::runtime_error("Failed to open shader file: " + path.string());

        std::stringstream ss;
        ss << shaderFile.rdbuf();
        outCode = ss.str();
        if (outCode.empty())
            throw std::runtime_error("Shader file is empty: " + path.string());
    }

	unsigned int compileSingleShader(const std::string& code, GLenum shaderType)
    {
        const char* cCode = code.c_str();
        unsigned int shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &cCode, nullptr);
        glCompileShader(shader);
        checkCompileErrors(shader, shaderType == GL_COMPUTE_SHADER ? "COMPUTE" : (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"));
        return shader;
    }

	unsigned int loadAndCompile(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	{
		std::string vertexCode, fragmentCode;

		readShaderFile(vertexPath, vertexCode);
        readShaderFile(fragmentPath, fragmentCode);

		unsigned int vertex = compileSingleShader(vertexCode, GL_VERTEX_SHADER);
		unsigned int fragment = compileSingleShader(fragmentCode, GL_FRAGMENT_SHADER);

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
			log::system_error("Shader", "[{}] Shader validation failed: {}", s_name, infoLog);
		}

		glDeleteShader(vertex);
		glDeleteShader(fragment);
		return program;
	}

	unsigned int loadAndCompile(const std::filesystem::path& computePath, ShaderType type)
	{
		if (type != ShaderType::Compute)
			log::system_error("Shader", "[{}]Invalid shader type for this overload", s_name);

		std::string computeCode;
		readShaderFile(computePath, computeCode);

		unsigned int computeShader = compileSingleShader(computeCode, GL_COMPUTE_SHADER);
		unsigned int program = glCreateProgram();
		glAttachShader(program, computeShader);
		glLinkProgram(program);
		checkCompileErrors(program, "PROGRAM");
		glDeleteShader(computeShader);

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
				log::system_error("Shader", "[{}] SHADER_COMPILATION_ERROR of type: {}\n{}", s_name, type, infoLog);
			}
		} else {
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success) {
				glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
				log::system_error("Shader", "[{}] PROGRAM_LINKING_ERROR of type: {}\n{}", s_name, type, infoLog);
			}
		}
	}
};
