#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "Shader.h"
#include "math/Face.h"
#include <vector>

class Mesh {
    unsigned int m_vertexArrayID;
    unsigned int m_vertexBufferID;
    unsigned int m_vertexCount;
    std::vector<Face> m_faces;

public:
    Mesh();
    ~Mesh();

    void setVertexData(unsigned int size, const void* data);
    unsigned int getVertexCount() const;
    void render(const Shader* shader) const;

private:
    void getFaces(const unsigned int* data);
};

#endif
