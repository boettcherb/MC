#include "Mesh.h"
#include "Constants.h"
#include "Shader.h"
#include "Face.h"
#include "BlockInfo.h"
#include <glad/glad.h>
#include <vector>

Mesh::Mesh() {
    m_vertexCount = 0;
    m_vertexArrayID = 0;
    m_vertexBufferID = 0;
    m_generated = false;
}

Mesh::~Mesh() {
    erase();
}

void Mesh::generate(unsigned int size, const void* data, bool setFaceData,
                    int chunkX, int chunkZ) {
    if (m_generated) {
        erase();
    }
    if (size == 0) {
        return;
    }
    glGenVertexArrays(1, &m_vertexArrayID);
    glGenBuffers(1, &m_vertexBufferID);

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
    if (setFaceData) {
        getFaces(reinterpret_cast<const unsigned int*>(data), chunkX, chunkZ);
    }

    m_generated = true;
}

bool Mesh::generated() const {
    return m_generated;
}

void Mesh::erase() {
    if (m_generated) {
        m_generated = false;
        m_vertexCount = 0;
        glDeleteVertexArrays(1, &m_vertexArrayID);
        glDeleteBuffers(1, &m_vertexBufferID);
        m_faces.clear();
    }
}

void Mesh::getFaces(const unsigned int* data, int chunkX, int chunkZ) {
    m_faces.reserve(m_vertexCount / VERTICES_PER_FACE);
    for (unsigned int i = 0; i < m_vertexCount; i += VERTICES_PER_FACE) {
        // each face has 6 vertices. However, the xyz coordinates of the 3rd
        // and 4th vertex are the same, as well as the 1st and 6th (seen in
        // Blockinfo.h). So take the 1st, 2nd, 3rd, and 5th vertex.
        sglm::vec3 offset = { (float) chunkX * CHUNK_WIDTH, 0.0f, (float) chunkZ * CHUNK_WIDTH };
        sglm::vec3 A = Block::getPosition(data[i + 0]) + offset;
        sglm::vec3 B = Block::getPosition(data[i + 1]) + offset;
        sglm::vec3 C = Block::getPosition(data[i + 2]) + offset;
        sglm::vec3 D = Block::getPosition(data[i + 4]) + offset;
        m_faces.emplace_back(Face(A, B, C, D));

    }
}

unsigned int Mesh::getVertexCount() const {
    return m_vertexCount;
}

void Mesh::render(const Shader* shader) const {
    if (m_generated) {
        shader->bind();
        glBindVertexArray(m_vertexArrayID);
        glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
    }
}

bool Mesh::intersects(const sglm::ray& ray, Face::Intersection& isect) {
    bool foundIntersection = false;
    Face::Intersection i;
    for (const Face& face : m_faces) {
        if (face.intersects(ray, i)) {
            if (!foundIntersection || i.t < isect.t) {
                foundIntersection = true;
                isect = i;
            }
        }
    }
    return foundIntersection;
}
