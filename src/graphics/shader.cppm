module;
#include <glad/glad.h>
//#include <string>
//#include <filesystem>
//#include <unordered_map>
//#include <mutex>
export module shader;

import std;
import glm;

export class Shader
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
	[[nodiscard]] inline unsigned int getProgramID() const noexcept { return ID; }
	[[nodiscard]] inline std::string getName() const noexcept { return s_name; }

	Shader(std::string _name, std::filesystem::path vertex_path, std::filesystem::path fragment_path);
	Shader(std::string _name, std::filesystem::path compute_path);
	Shader(const Shader&)            = delete;
	Shader& operator=(const Shader&) = delete;

	~Shader() noexcept;

	void use() const noexcept;

	bool checkAndReloadIfModified();

	bool reload();

	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setUInt(const std::string& name, unsigned int value) const;
	void setFloat(const std::string& name, float value) const;
	void setVec2(const std::string& name, const glm::vec2& value) const;
	void setVec3(const std::string& name, const glm::vec3& value) const;
	void setIVec3(const std::string& name, const glm::ivec3& value) const;
	void setUVec3(const std::string& name, const glm::uvec3& value) const;
	void setVec4(const std::string& name, const glm::vec4& value) const;
	void setMat4(const std::string& name, const glm::mat4& mat) const;

      private:
	enum class ShaderType { VertexFragment, Compute };
	void reflectUniforms();

	GLint getUniformLocation(const std::string& uni_name) const;

	void readShaderFile(const std::filesystem::path& path, std::string& outCode);

	unsigned int compileSingleShader(const std::string& code, GLenum shaderType);

	unsigned int loadAndCompile(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
	unsigned int loadAndCompile(const std::filesystem::path& computePath, ShaderType type);

	void checkCompileErrors(unsigned int shader, const std::string& type);
};
