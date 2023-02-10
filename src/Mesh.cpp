#include "Mesh.h"
#include "Shader.h"
#include "math/Face.h"
#include <glad/glad.h>
#include <vector>

Mesh::Mesh() : m_vertexCount{ 0 } {
    glGenVertexArrays(1, &m_vertexArrayID);
    glGenBuffers(1, &m_vertexBufferID);
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_vertexArrayID);
    glDeleteBuffers(1, &m_vertexBufferID);
}

void Mesh::setVertexData(unsigned int size, const void* data) {
    if (size == 0) {
        return;
    }

    // bind both buffers (vertex array first)
    glBindVertexArray(m_vertexArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferID);

    // set up memory location for vertex data and pass in the data
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);

    // tell openGL the layout of our vertex data
    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(unsigned int), 0);

    // store the number of vertices
    m_vertexCount = size / sizeof(unsigned int);

    // set the face data (used for collisions)
    getFaces(reinterpret_cast<const unsigned int*>(data));
}

void Mesh::getFaces(const unsigned int* data) {
    m_faces.reserve(m_vertexCount / 6);
    for (unsigned int i = 0; i < m_vertexCount; i += 6) {
        // each face has 6 vertices. However, the xyz coordinates of the 3rd
        // and 4th vertex are the same, as well as the 1st and 6th (seen in
        // Blockinfo.h). So take the 1st, 2nd, 3rd, and 5th vertex.
        unsigned int Ax = (data[i] >> 23) & 0x1F;
        unsigned int Ay = (data[i] >> 15) & 0xFF;
        unsigned int Az = (data[i] >> 10) & 0x1F;
        unsigned int Bx = (data[i + 1] >> 23) & 0x1F;
        unsigned int By = (data[i + 1] >> 15) & 0xFF;
        unsigned int Bz = (data[i + 1] >> 10) & 0x1F;
        unsigned int Cx = (data[i + 2] >> 23) & 0x1F;
        unsigned int Cy = (data[i + 2] >> 15) & 0xFF;
        unsigned int Cz = (data[i + 2] >> 10) & 0x1F;
        unsigned int Dx = (data[i + 4] >> 23) & 0x1F;
        unsigned int Dy = (data[i + 4] >> 15) & 0xFF;
        unsigned int Dz = (data[i + 4] >> 10) & 0x1F;
        sglm::vec3 A = { (float) Ax, (float) Ay, (float) Az };
        sglm::vec3 B = { (float) Bx, (float) By, (float) Bz };
        sglm::vec3 C = { (float) Cx, (float) Cy, (float) Cz };
        sglm::vec3 D = { (float) Dx, (float) Dy, (float) Dz };
        m_faces.emplace_back(Face(A, B, C, D));
    }
}

unsigned int Mesh::getVertexCount() const {
    return m_vertexCount;
}

void Mesh::render(const Shader* shader) const {
    shader->bind();
    glBindVertexArray(m_vertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}
