#ifndef SHADER_H_INCLUDED
#define SHADER_H_INCLUDED

#include "Texture.h"
#include <sglm/sglm.h>
#include <string>
#include <vector>

class Shader {
    unsigned int m_shaderID;
    int m_uniforms[4];

public:
    Shader(const std::string& vertexFilePath, const std::string& fragmentFilePath);
    ~Shader();

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
