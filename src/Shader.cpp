#include "Shader.h"
#include "Texture.h"
#include <glad/glad.h>
#include <math/sglm.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>

Shader::Shader(const std::string& vertexFilePath, const std::string& fragmentFilePath) {
    m_shaderID = glCreateProgram();
    unsigned vertexID = glCreateShader(GL_VERTEX_SHADER);
    unsigned fragmentID = glCreateShader(GL_FRAGMENT_SHADER);
    compile(vertexID, parseShader(vertexFilePath).c_str());
    compile(fragmentID, parseShader(fragmentFilePath).c_str());
    glLinkProgram(m_shaderID);

#ifndef NDEBUG
    // make sure the shader program linked successfully
    int success = 0;
    glValidateProgram(m_shaderID);
    glGetProgramiv(m_shaderID, GL_VALIDATE_STATUS, &success);
    if (!success) {
        std::cerr << "Shader Program Linking Failed\n";
    }
#endif

    // the individual shaders are not needed after they have been linked into one program
    glDeleteShader(vertexID);
    glDeleteShader(fragmentID);

    // initialize values in m_uniforms array to -1
    std::memset(m_uniforms, -1, 16);
}

Shader::~Shader() {
    glDeleteProgram(m_shaderID);
}

void Shader::compile(unsigned id, const char* source) const {
    glShaderSource(id, 1, &source, nullptr);
    glCompileShader(id);

#ifndef NDEBUG
    // make sure the shader compiled successfully
    int success = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512] = { 0 };
        glGetShaderInfoLog(id, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Failed\n" << infoLog << '\n';
    }
#endif

    // combine each individual shader into one program
    glAttachShader(m_shaderID, id);
    
}

std::string Shader::parseShader(const std::string& filePath) const {
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

void Shader::bind() const {
    glUseProgram(m_shaderID);
}

void Shader::unbind() const {
    glUseProgram(0);
}

void Shader::addTexture(const Texture* texture, const std::string& name) {
    bind();
    texture->bind();
    addUniform1i(name, texture->getSlot());
}

void Shader::addUniform1i(const std::string& name, int v0) {
    bind();
    glUniform1i(getUniformLocation(name), v0);
}

void Shader::addUniformMat4f(const std::string& name, const sglm::mat4& matrix) {
    bind();
    glUniformMatrix4fv(getUniformLocation(name), 1, GL_FALSE, reinterpret_cast<const GLfloat*>(&matrix));
}

int Shader::getUniformLocation(const std::string& name) {
    assert(name.length() > 3 && name[0] == 'u' && name[1] >= '0' && name[1] <= '9' && name[2] == '_');
    unsigned index = name[1] - '0';
    if (m_uniforms[index] != -1) {
        return m_uniforms[index];
    }
    m_uniforms[index] = glGetUniformLocation(m_shaderID, name.c_str());
    assert(m_uniforms[index] != -1);
    return m_uniforms[index];
}
