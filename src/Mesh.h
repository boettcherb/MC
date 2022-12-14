#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "Shader.h"

class Mesh {
    unsigned int m_vertexArrayID;
    unsigned int m_vertexBufferID;
    unsigned int m_vertexCount;

public:
    Mesh();
    ~Mesh();

    void setVertexData(unsigned int size, const void* data);
    void render(const Shader* shader) const;
};

#endif
