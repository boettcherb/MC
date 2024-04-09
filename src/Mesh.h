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

    void generate(unsigned int size, const void* data, bool setFaceData,
                  int cx = 0, int cy = 0, int cz = 0);
    bool generated() const;
    void erase();
    unsigned int getVertexCount() const;
    bool render(const Shader* shader) const;
    bool intersects(const sglm::ray& ray, Face::Intersection& isect);

private:
    void getFaces(const vertex_attrib_t* data, int cx, int cy, int cz);
};

#endif
