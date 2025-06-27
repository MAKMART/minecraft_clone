#pragma once
#include <GL/glew.h>
#include <stdexcept>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include "logger.hpp"
class Shader {
  private:
    unsigned int ID; // Shader program ID
  public:
    unsigned int getProgramID(void) const { return ID; };

    // Constructor with filesystem::path : Reads and builds the shader
    Shader(const std::filesystem::path &vertexPath, const std::filesystem::path &fragmentPath) {
        // 1. Retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // Ensure ifstream objects can throw exceptions
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // Open files using the std::filesystem::path
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);

            std::stringstream vShaderStream, fShaderStream;

            // Read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // Close file handlers
            vShaderFile.close();
            fShaderFile.close();

            // Convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            if (vertexCode.empty() || fragmentCode.empty()) {
                throw std::runtime_error("ERROR::SHADER::SOURCE_CODE_EMPTY\n");
                return;
            }
        } catch (std::ifstream::failure &e) {
            throw std::runtime_error("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ\n");
            return;
        }

        const GLchar *vShaderCode = vertexCode.c_str();
        const GLchar *fShaderCode = fragmentCode.c_str();

        // 2. Compile shaders
        unsigned int vertex, fragment;

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Validate the shader program after linking
        glValidateProgram(ID);
        GLint programSuccess;
        glGetProgramiv(ID, GL_VALIDATE_STATUS, &programSuccess);
        if (!programSuccess) {
            char infoLog[1024];
            glGetProgramInfoLog(ID, 1024, nullptr, infoLog);
            log::system_error("Shader", "PROGRAM_VALIDATION_FAILED\n{}", infoLog);
            return;
        }

        // Delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // Constructor with C strings: Reads and builds the shader
    Shader(const char *vertexPath, const char *fragmentPath) {
        // 1. Retrieve the vertex/fragment source code from filePath
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // Ensure ifstream objects can throw exceptions
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            // Open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);

            std::stringstream vShaderStream, fShaderStream;

            // Read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            // Close file handlers
            vShaderFile.close();
            fShaderFile.close();

            // Convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
            if (vertexCode.empty() || fragmentCode.empty()) {
                throw std::runtime_error("[Shader] [ERROR] SOURCE_CODE_EMPTY\n");
                return;
            }
        } catch (std::ifstream::failure &e) {
            throw std::runtime_error("[Shader] [ERROR] FILE_NOT_SUCCESFULLY_READ\n");
            return;
        }

        const GLchar *vShaderCode = vertexCode.c_str();
        const GLchar *fShaderCode = fragmentCode.c_str();

        // std::cout << "Vertex shader path: " << vertexPath << "\n";
        // std::cout << "Fragment shader path: " << fragmentPath << "\n";
        //  2. Compile shaders
        unsigned int vertex, fragment;

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, nullptr);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, nullptr);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Validate the shader program after linking
        glValidateProgram(ID);
        GLint programSuccess;
        glGetProgramiv(ID, GL_VALIDATE_STATUS, &programSuccess);
        if (!programSuccess) {
            char infoLog[1024];
            glGetProgramInfoLog(ID, 1024, nullptr, infoLog);
            log::system_error("Shader", "PROGRAM_VALIDATION_FAILED\n{}", infoLog);
            return;
        }

        // Delete the shaders as they're linked into our program now and no longer necessary
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
    ~Shader(void) {
        if (glIsProgram(ID))
            glDeleteProgram(ID);
    }
    // Activate the shader
    void use(void) const {
        if (glIsProgram(ID)) {
            glUseProgram(ID);
        } else {
            log::system_error("Shader", "Program is not valid! ID = {}", ID);
        }
    }

    // Instead of calling glUseProgram, you directly use the program ID in other functions
    void setBool(const std::string &name, bool value) const {
        GLint location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            log::system_warn("Shader", "Warning: Uniform {} not found in shader program!", name);
        } else {
            glProgramUniform1i(ID, location, static_cast<int>(value));
        }
    }

    void setInt(const std::string &name, int value) const {
        GLint location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            log::system_warn("Shader", "Warning: Uniform {} not found in shader program!", name);
        } else {
            glProgramUniform1i(ID, location, value);
        }
    }

    void setFloat(const std::string &name, float value) const {
        GLint location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            log::system_warn("Shader", "Warning: Uniform {} not found in shader program!", name);
        } else {
            glProgramUniform1f(ID, location, value);
        }
    }
    void setVec2(const std::string &name, const glm::vec2 &value) {
        GLint location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            log::system_warn("Shader", "Warning: Uniform {} not found in shader program!", name);
        } else {
            glProgramUniform2fv(ID, location, 1, glm::value_ptr(value));
        }
    }

    void setVec3(const std::string &name, const glm::vec3 &value) const {
        GLint location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            log::system_warn("Shader", "Warning: Uniform {} not found in shader program!", name);
        } else {
            glProgramUniform3fv(ID, location, 1, glm::value_ptr(value));
        }
    }

    void setVec4(const std::string &name, const glm::vec4 &value) const {
        GLint location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            log::system_warn("Shader", "Warning: Uniform {} not found in shader program!", name);
        } else {
            glProgramUniform4fv(ID, location, 1, glm::value_ptr(value));
        }
    }

    void setMat4(const std::string &name, const glm::mat4 &mat) const {
        GLint location = glGetUniformLocation(ID, name.c_str());
        if (location == -1) {
            log::system_warn("Shader", "Warning: Uniform {} not found in shader program!", name);
        } else {
            glProgramUniformMatrix4fv(ID, location, 1, GL_FALSE, glm::value_ptr(mat));
        }
    }

  private:
    // Utility function for checking shader compilation/linking errors
    void checkCompileErrors(unsigned int shader, const std::string &type) {
        int success;
        char infoLog[1024];

        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
                std::string error = "SHADER_COMPILATION_ERROR of type: " + type + "\n" + infoLog;
                throw std::runtime_error(error);
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, nullptr, infoLog);
                std::string error = "PROGRAM_LINKING_ERROR of type: " + type + "\n" + infoLog;
                throw std::runtime_error(error);
            }
        }
    }
};
