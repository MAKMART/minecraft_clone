module;
#include <glad/gl.h>
export module engine.renderer:gl_backend.shader;

import std;
import engine.core;
import engine.math;

using namespace engine::math;
export namespace engine::renderer {
  class Shader {
    public:
      Shader(std::string _name, std::filesystem::path vertex_path, std::filesystem::path fragment_path);
      Shader(std::string _name, std::filesystem::path compute_path);
      Shader(const Shader&) = delete;
      Shader& operator=(const Shader&) = delete;
      ~Shader() noexcept;

      [[nodiscard]] inline u32 id() const noexcept { return ID; }
      [[nodiscard]] inline std::string get_name() const noexcept { return s_name; }

      void use() const noexcept;
      bool checkAndReloadIfModified();
      bool reload();

      void set_bool(const std::string& name, bool value) const;
      void set_i32(const std::string& name, i32 value) const;
      void set_u32(const std::string& name, u32 value) const;
      void set_f32(const std::string& name, f32 value) const;
      void set_vec2(const std::string& name, const vec2& value) const;
      void set_vec3(const std::string& name, const vec3& value) const;
      void set_vec4(const std::string& name, const vec4& value) const;
      void set_ivec3(const std::string& name, const ivec3& value) const;
      void set_uvec3(const std::string& name, const uvec3& value) const;
      void set_mat4(const std::string& name, const mat4& mat) const;

    private:
      u32 ID;
      std::string s_name;
      std::filesystem::path vertexShaderPath;
      std::filesystem::path fragmentShaderPath;
      std::filesystem::path computeShaderPath;
      mutable std::unordered_map<std::string, GLint> uniformCache;
      mutable std::mutex uniformCacheMutex;

      std::filesystem::file_time_type lastVertexWriteTime;
      std::filesystem::file_time_type lastFragmentWriteTime;
      std::filesystem::file_time_type lastComputeWriteTime;
      enum class ShaderType { VertexFragment, Compute };
      void reflectUniforms();

      GLint getUniformLocation(const std::string& uni_name) const;
      void readShaderFile(const std::filesystem::path& path, std::string& outCode);
      u32 compileSingleShader(const std::string& code, GLenum shaderType);
      u32 loadAndCompile(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
      u32 loadAndCompile(const std::filesystem::path& computePath, ShaderType type);
      void checkCompileErrors(unsigned int shader, const std::string& type);
  };

}
namespace engine::renderer {

  Shader::Shader(std::string _name, std::filesystem::path vertex_path, std::filesystem::path fragment_path)
    : s_name(std::move(_name)), vertexShaderPath(std::move(vertex_path)), fragmentShaderPath(std::move(fragment_path))
  {
    ID = loadAndCompile(vertexShaderPath, fragmentShaderPath);
    reflectUniforms();
    lastVertexWriteTime   = std::filesystem::last_write_time(vertexShaderPath);
    lastFragmentWriteTime = std::filesystem::last_write_time(fragmentShaderPath);
  }

  Shader::Shader(std::string _name, std::filesystem::path compute_path)
    : s_name(std::move(_name)), computeShaderPath(std::move(compute_path))
  {
    ID = loadAndCompile(computeShaderPath, ShaderType::Compute);
    reflectUniforms();
    lastComputeWriteTime = std::filesystem::last_write_time(computeShaderPath);
  }

  Shader::~Shader() noexcept {
    if (glIsProgram(ID)) {
      glDeleteProgram(ID);
    }
  }

  void Shader::use() const noexcept {
    if (glIsProgram(ID)) {
      glUseProgram(ID);
    } else {
      engine::core::logger::system_error("Shader", "[{}] Program is not valid! ID = {}", s_name, ID);
    }
  }

  bool Shader::checkAndReloadIfModified() {
    try {
      auto currentVertexTime   = std::filesystem::last_write_time(vertexShaderPath);
      auto currentFragmentTime = std::filesystem::last_write_time(fragmentShaderPath);

      if (currentVertexTime != lastVertexWriteTime || currentFragmentTime != lastFragmentWriteTime) {
        lastVertexWriteTime   = currentVertexTime;
        lastFragmentWriteTime = currentFragmentTime;
        return reload();
      }
    } catch (const std::exception& e) {
      engine::core::logger::system_warn("Shader", "[{}] Error checking shader file timestamps: {}", s_name, e.what());
    }
    return false;
  }

  bool Shader::reload() {
    try {
      unsigned int newProgram = loadAndCompile(vertexShaderPath, fragmentShaderPath);

      if (glIsProgram(ID)) {
        glDeleteProgram(ID);
      }

      ID = newProgram;
      uniformCache.clear(); // Uniforms may have changed
      reflectUniforms();

      engine::core::logger::system_info("Shader", "[{}] Reloaded shader from {} and {}", s_name, vertexShaderPath.string(), fragmentShaderPath.string());
      return true;
    } catch (const std::exception& e) {
      engine::core::logger::system_error("Shader", "[{}] Failed to reload shader: {}", s_name, e.what());
      return false;
    }
  }

  void Shader::set_bool(const std::string& name, bool value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform1i(ID, location, static_cast<int>(value));
  }

  void Shader::set_i32(const std::string& name, i32 value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform1i(ID, location, value);
  }

  void Shader::set_u32(const std::string& name, u32 value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform1ui(ID, location, value);
  }

  void Shader::set_f32(const std::string& name, f32 value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform1f(ID, location, value);
  }

  void Shader::set_vec2(const std::string& name, const vec2& value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform2fv(ID, location, 1, value_ptr(value));
  }

  void Shader::set_vec3(const std::string& name, const vec3& value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform3fv(ID, location, 1, value_ptr(value));
  }

  void Shader::set_vec4(const std::string& name, const vec4& value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform4fv(ID, location, 1, value_ptr(value));
  }

  void Shader::set_ivec3(const std::string& name, const ivec3& value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform3iv(ID, location, 1, value_ptr(value));
  }


  void Shader::set_uvec3(const std::string& name, const uvec3& value) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }

    glProgramUniform3uiv(ID, location, 1, value_ptr(value));
  }

  void Shader::set_mat4(const std::string& name, const mat4& mat) const {
    if (ID == 0) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Attempting to set uniform '{}' on invalid shader program!", s_name, name);
#endif
      return;
    }

    GLint location = getUniformLocation(name);
    if (location == -1) {
#if defined(DEBUG)
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, name);
#endif
      return;
    }
    glProgramUniformMatrix4fv(ID, location, 1, GL_FALSE, value_ptr(mat));
  }

  void Shader::reflectUniforms() {
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
        uniformCache.try_emplace(std::move(uni_name), location);
      }
    }
    if (!uniformCache.empty())
      engine::core::logger::system_info("Shader", "[{}] Reflected and cached {} active uniforms.", s_name, uniformCache.size());
  }

  GLint Shader::getUniformLocation(const std::string& uni_name) const {
    std::lock_guard<std::mutex> lock(uniformCacheMutex);
    auto                        it = uniformCache.find(uni_name);
    if (it != uniformCache.end()) {
      return it->second;
    }
    GLint location = glGetUniformLocation(ID, uni_name.c_str());
    if (location == -1) {
      engine::core::logger::system_warn("Shader", "[{}] Uniform {} not found in shader program!", s_name, uni_name);
    } else {
      uniformCache.try_emplace(uni_name, location);
    }
    return location;
  }

  void Shader::readShaderFile(const std::filesystem::path& path, std::string& outCode) {
    std::ifstream shaderFile(path);
    if (!shaderFile.is_open())
      throw std::runtime_error("Failed to open shader file: '" + path.string() + "'");

    std::stringstream ss;
    ss << shaderFile.rdbuf();
    outCode = ss.str();
    if (outCode.empty())
      throw std::runtime_error("Shader file is empty: '" + path.string() + "'");
  }

  u32 Shader::compileSingleShader(const std::string& code, GLenum shaderType) {
    const char* cCode = code.c_str();
    u32 shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &cCode, nullptr);
    glCompileShader(shader);
    checkCompileErrors(shader, shaderType == GL_COMPUTE_SHADER ? "COMPUTE" : (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT"));
    return shader;
  }

  u32 Shader::loadAndCompile(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath) {
    std::string vertexCode, fragmentCode;

    readShaderFile(vertexPath, vertexCode);
    readShaderFile(fragmentPath, fragmentCode);

    u32 vertex = compileSingleShader(vertexCode, GL_VERTEX_SHADER);
    u32 fragment = compileSingleShader(fragmentCode, GL_FRAGMENT_SHADER);

    u32 program = glCreateProgram();
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
      engine::core::logger::system_error("Shader", "[{}] Shader validation failed: {}", s_name, infoLog);
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
  }

  u32 Shader::loadAndCompile(const std::filesystem::path& computePath, ShaderType type) {
    if (type != ShaderType::Compute)
      engine::core::logger::system_error("Shader", "[{}] Invalid shader type for this overload", s_name);

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

  void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    int  success;
    char infoLog[1024];

    if (type != "PROGRAM") {
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
        engine::core::logger::system_error("Shader", "[{}] SHADER_COMPILATION_ERROR of type: {}\n{}", s_name, type, infoLog);
      }
    } else {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
        engine::core::logger::system_error("Shader", "[{}] PROGRAM_LINKING_ERROR of type: {}\n{}", s_name, type, infoLog);
      }
    }
  }
}
