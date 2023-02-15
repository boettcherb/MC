#ifndef MESH_H_INCLUDED
#define MESH_H_INCLUDED

#include "Shader.h"
#include "Face.h"
#include <vector>

class Mesh {
    bool m_generated;
    unsigned int m_vertexArrayID;
    unsigned int m_vertexBufferID;
    unsigned int m_vertexCount;
    std::vector<Face> m_faces;

public:
    Mesh();
    ~Mesh();

    void generate(unsigned int size, const void* data, bool getFaceData);
    bool generated() const;
    void erase();
    unsigned int getVertexCount() const;
    void render(const Shader* shader) const;
    Face* intersects(const Ray& ray);

private:
    void getFaces(const unsigned int* data);
};

#endif
