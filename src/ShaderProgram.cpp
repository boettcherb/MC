#include "ShaderProgram.h"
#include "Texture.h"
#include <glad/glad.h>
#include <sglm/sglm.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <unordered_map>

ShaderProgram::ShaderProgram(const std::string& vertexFilePath, const std::string& fragmentFilePath) {
    m_shaderProgramID = glCreateProgram();
    unsigned vertexID = glCreateShader(GL_VERTEX_SHADER);
    unsigned fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
    compile(vertexID, parseShader(vertexFilePath).c_str());
    compile(fragmentID, parseShader(fragmentFilePath).c_str());
    glLinkProgram(m_shaderProgramID);

#ifndef NDEBUG
    // make sure the shader program linked successfully
    int success = 0;
    glValidateProgram(m_shaderProgramID);
    glGetProgramiv(m_shaderProgramID, GL_VALIDATE_STATUS, &success);
    if (!success) {
        std::cerr << "Shader Program Linking Failed\n";
    }
#endif

    // the individual shaders are not needed after they have been linked into one program
    glDeleteShader(vertexID);
    glDeleteShader(fragmentID);
}

ShaderProgram::~ShaderProgram() {
    glDeleteProgram(m_shaderProgramID);
}

void ShaderProgram::compile(unsigned id, const char* source) const {
    int success = 0;
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

#ifndef NDEBUG
    // make sure the shader compiled successfully
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512] = { 0 };
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Failed\n" << infoLog << '\n';
    }
#endif

    // combine each individual shader into one program
    glAttachShader(m_shaderProgramID, id);
    
}

std::string ShaderProgram::parseShader(const std::string& filePath) const {
    std::ifstream stream(filePath);
    assert(stream);
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

void ShaderProgram::addUniformMat4f(const std::string& name, const sglm::mat4& matrix) {
    bind();
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&matrix));
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
