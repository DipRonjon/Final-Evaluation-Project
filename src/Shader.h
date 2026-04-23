#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

/**
 * Shader
 * ------
 * Loads, compiles and links a vertex + fragment shader pair from disk.
 * Provides convenient uniform setters used throughout the scene.
 */
class Shader
{
public:
    GLuint ID;

    Shader(const char* vertexPath, const char* fragmentPath);

    void use() const;

    void setInt  (const std::string& name, int              value) const;
    void setFloat(const std::string& name, float            value) const;
    void setVec3 (const std::string& name, const glm::vec3& value) const;
    void setMat4 (const std::string& name, const glm::mat4& value) const;

private:
    static void checkCompileErrors(GLuint object, const std::string& type);
};
