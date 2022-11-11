#include "ShaderProgram.h"
#include "Texture.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

ShaderProgram::Shader::Shader(unsigned int id, const std::string& source)
    : m_id{ id }, m_source{ source } {}

ShaderProgram::ShaderProgram(const std::string& vertexFilePath, const std::string& fragmentFilePath) {
    m_shaderProgramID = glCreateProgram();
    m_shaders.emplace_back(glCreateShader(GL_VERTEX_SHADER), parseShader(vertexFilePath));
    m_shaders.emplace_back(glCreateShader(GL_FRAGMENT_SHADER), parseShader(fragmentFilePath));
    compileAndLink();

    // the individual shaders are not needed after they have been linked into one program
    for (const ShaderProgram::Shader& shader : m_shaders) {
        glDeleteShader(shader.m_id);
    }
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(m_shaderProgramID);
}

void ShaderProgram::compileAndLink() const {
    int success = 0;
    for (const ShaderProgram::Shader& shader : m_shaders) {
        const char* src = shader.m_source.c_str();
        glShaderSource(shader.m_id, 1, &src, nullptr);
        glCompileShader(shader.m_id);

        // make sure the shader compiled successfully
        glGetShaderiv(shader.m_id, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512] = { 0 };
            glGetShaderInfoLog(shader.m_id, 512, nullptr, infoLog);
            std::cerr << "Shader Compilation Failed\n" << infoLog << '\n';
        }

        // combine each individual shader into one program
        glAttachShader(m_shaderProgramID, shader.m_id);
    }

    glLinkProgram(m_shaderProgramID);

    // make sure the shader program linked successfully
    glValidateProgram(m_shaderProgramID);
    glGetProgramiv(m_shaderProgramID, GL_VALIDATE_STATUS, &success);
    if (!success) {
        std::cerr << "Shader Program Linking Failed\n";
    }
}

std::string ShaderProgram::parseShader(const std::string& filePath) const {
    std::ifstream stream(filePath);
    if (!stream) {
        std::cerr << "Could not find/open shader at " << filePath << '\n';
    }
    std::string shaderSource;
    while (stream) {
        std::string line;
        std::getline(stream, line);
        shaderSource += line + '\n';
    }
    return shaderSource;
}

void ShaderProgram::bind() const {
    glUseProgram(m_shaderProgramID);
}

void ShaderProgram::unbind() const {
    glUseProgram(0);
}

void ShaderProgram::addTexture(const Texture* texture, const std::string& name) {
    bind();
    texture->bind();
    addUniform1i(name, texture->getSlot());
}

void ShaderProgram::addUniform1i(const std::string& name, int v0) {
    bind();
    glUniform1i(getUniformLocation(name), v0);
}

void ShaderProgram::addUniform2i(const std::string& name, int v0, int v1) {
    bind();
    glUniform2i(getUniformLocation(name), v0, v1);
}

void ShaderProgram::addUniform1f(const std::string& name, float v0) {
    bind();
    glUniform1f(getUniformLocation(name), v0);
}

void ShaderProgram::addUniform2f(const std::string& name, float v0, float v1) {
    bind();
    glUniform2f(getUniformLocation(name), v0, v1);
}

void ShaderProgram::addUniform3f(const std::string& name, float v0, float v1, float v2) {
    bind();
    glUniform3f(getUniformLocation(name), v0, v1, v2);
}

void ShaderProgram::addUniform4f(const std::string& name, float v0, float v1, float v2, float v3) {
    bind();
    glUniform4f(getUniformLocation(name), v0, v1, v2, v3);
}

void ShaderProgram::addUniformMat4f(const std::string& name, const glm::mat4& matrix) {
    bind();
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, glm::value_ptr(matrix));
}

int ShaderProgram::getUniformLocation(const std::string& name) {
    auto cachedLocation = m_uniformLocationCache.find(name);
    if (cachedLocation != m_uniformLocationCache.end()) {
        return cachedLocation->second;
    }
    int location = glGetUniformLocation(m_shaderProgramID, name.c_str());
    if (location == -1) {
        std::cerr << "The Uniform " + name + " does not exist!\n";
    }
    m_uniformLocationCache[name] = location;
    return location;
}
