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
    glVertexAttribIPointer(0, ATTRIBS_PER_VERTEX, GL_UNSIGNED_SHORT, VERTEX_SIZE, 0);

    // store the number of vertices
    m_vertexCount = size / VERTEX_SIZE;

    // set the face data (used for collisions)
    if (setFaceData) {
        getFaces(reinterpret_cast<const VertexAttribType*>(data), chunkX, chunkZ);
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

void Mesh::getFaces(const VertexAttribType* data, int chunkX, int chunkZ) {
    m_faces.reserve(m_vertexCount / VERTICES_PER_FACE);
    for (unsigned int i = 0; i < m_vertexCount * ATTRIBS_PER_VERTEX; i += ATTRIBS_PER_FACE) {
        // each face has 6 vertices. However, the xyz coordinates of the 3rd
        // and 4th vertex are the same, as well as the 1st and 6th (seen in
        // Blockinfo.h). So take the 1st, 2nd, 3rd, and 5th vertex.
        Vertex v1, v2, v3, v4;
        std::memcpy(&v1, &data[i], sizeof(Vertex));
        std::memcpy(&v2, &data[i + 3], sizeof(Vertex));
        std::memcpy(&v3, &data[i + 6], sizeof(Vertex));
        std::memcpy(&v4, &data[i + 12], sizeof(Vertex));

        sglm::vec3 offset = { (float) chunkX * CHUNK_WIDTH, 0.0f, (float) chunkZ * CHUNK_WIDTH };
        sglm::vec3 A = Block::getVertexPosition(v1) + offset;
        sglm::vec3 B = Block::getVertexPosition(v2) + offset;
        sglm::vec3 C = Block::getVertexPosition(v3) + offset;
        sglm::vec3 D = Block::getVertexPosition(v4) + offset;
        sglm::vec3 blockPosition = Block::getBlockPosition(v1);
        m_faces.emplace_back(Face(A, B, C, D, blockPosition));
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
    for (const Face& face : m_faces) {
        Face::Intersection i;
        if (face.intersects(ray, i)) {
            if (!foundIntersection || i.t < isect.t) {
                foundIntersection = true;
                isect = i;
            }
        }
    }
    return foundIntersection;
}
