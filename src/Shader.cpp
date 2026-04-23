#include "Shader.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // 1. Read source files
    auto readFile = [](const char* path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open())
            throw std::runtime_error(std::string("Cannot open shader file: ") + path);
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    };

    std::string vertSrc = readFile(vertexPath);
    std::string fragSrc = readFile(fragmentPath);

    const char* vCode = vertSrc.c_str();
    const char* fCode = fragSrc.c_str();

    // 2. Compile vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vCode, nullptr);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // 3. Compile fragment shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fCode, nullptr);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // 4. Link program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() const
{
    glUseProgram(ID);
}

void Shader::setInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

void Shader::setMat4(const std::string& name, const glm::mat4& value) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1,
                       GL_FALSE, glm::value_ptr(value));
}

void Shader::checkCompileErrors(GLuint object, const std::string& type)
{
    GLint  success;
    GLchar infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(object, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "[Shader] Compile error (" << type << "):\n"
                      << infoLog << "\n";
        }
    } else {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(object, sizeof(infoLog), nullptr, infoLog);
            std::cerr << "[Shader] Link error:\n" << infoLog << "\n";
        }
    }
}
