#ifndef SHADER_PROGRAM_H_INCLUDED
#define SHADER_PROGRAM_H_INCLUDED

#include "Texture.h"

#include <sglm/sglm.h>

#include <string>
#include <vector>
#include <unordered_map>

class ShaderProgram {
    unsigned int m_shaderProgramID;
    std::unordered_map<std::string, int> m_uniformLocationCache;

public:
    ShaderProgram(const std::string& vertexFilePath, const std::string& fragmentFilePath);
    ~ShaderProgram();

    void bind() const;
    void unbind() const;
    void addTexture(const Texture* texture, const std::string& name);
    void addUniform1i(const std::string& name, int v0);
    void addUniformMat4f(const std::string& name, const sglm::mat4& matrix);

private:
    void compile(unsigned id, const char* source) const;
    std::string parseShader(const std::string& filePath) const;
    int getUniformLocation(const std::string& name);
};

#endif
