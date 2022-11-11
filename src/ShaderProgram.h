#ifndef SHADER_PROGRAM_H_INCLUDED
#define SHADER_PROGRAM_H_INCLUDED

#include "Texture.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <unordered_map>

class ShaderProgram {
    
    struct Shader {
        unsigned int m_id;
        std::string m_source;
        Shader(unsigned int id, const std::string& source);
    };

    unsigned int m_shaderProgramID;
    std::vector<ShaderProgram::Shader> m_shaders;
    std::unordered_map<std::string, int> m_uniformLocationCache;

public:
    ShaderProgram(const std::string& vertexFilePath, const std::string& fragmentFilePath);
    ~ShaderProgram();

    void bind() const;
    void unbind() const;
    void addTexture(const Texture* texture, const std::string& name);

    void addUniform1i(const std::string& name, int v0);
    void addUniform2i(const std::string& name, int v0, int v1);
    void addUniform1f(const std::string& name, float v0);
    void addUniform2f(const std::string& name, float v0, float v1);
    void addUniform3f(const std::string& name, float v0, float v1, float v2);
    void addUniform4f(const std::string& name, float v0, float v1, float v2, float v3);
    void addUniformMat4f(const std::string& name, const glm::mat4& matrix);

private:
    void compileAndLink() const;
    std::string parseShader(const std::string& filePath) const;
    int getUniformLocation(const std::string& name);
};

#endif
