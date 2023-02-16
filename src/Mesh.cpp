#include "Mesh.h"
#include "Constants.h"
#include "Shader.h"
#include "Face.h"
#include <glad/glad.h>
#include <vector>







// #include <iostream>





Mesh::Mesh() {
    m_vertexCount = 0;
    m_vertexArrayID = 0;
    m_vertexBufferID = 0;
    m_generated = false;
}

Mesh::~Mesh() {
    erase();
}

void Mesh::generate(unsigned int size, const void* data, bool getFaceData) {
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
    if (getFaceData) {
        getFaces(reinterpret_cast<const unsigned int*>(data));
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

void Mesh::getFaces(const unsigned int* data) {
    m_faces.reserve(m_vertexCount / VERTICES_PER_FACE);
    for (unsigned int i = 0; i < m_vertexCount; i += VERTICES_PER_FACE) {
        // each face has 6 vertices. However, the xyz coordinates of the 3rd
        // and 4th vertex are the same, as well as the 1st and 6th (seen in
        // Blockinfo.h). So take the 1st, 2nd, 3rd, and 5th vertex.
        float Ax = (float) ((data[i] >> 23) & 0x1F);
        float Ay = (float) ((data[i] >> 15) & 0xFF);
        float Az = (float) ((data[i] >> 10) & 0x1F);
        float Bx = (float) ((data[i + 1] >> 23) & 0x1F);
        float By = (float) ((data[i + 1] >> 15) & 0xFF);
        float Bz = (float) ((data[i + 1] >> 10) & 0x1F);
        float Cx = (float) ((data[i + 2] >> 23) & 0x1F);
        float Cy = (float) ((data[i + 2] >> 15) & 0xFF);
        float Cz = (float) ((data[i + 2] >> 10) & 0x1F);
        float Dx = (float) ((data[i + 4] >> 23) & 0x1F);
        float Dy = (float) ((data[i + 4] >> 15) & 0xFF);
        float Dz = (float) ((data[i + 4] >> 10) & 0x1F);
        sglm::vec3 A = { Ax, Ay, Az };
        sglm::vec3 B = { Bx, By, Bz };
        sglm::vec3 C = { Cx, Cy, Cz };
        sglm::vec3 D = { Dx, Dy, Dz };
        m_faces.emplace_back(Face(A, B, C, D, data + i));
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

Face* Mesh::intersects(const sglm::ray& ray) {
    Face* closestFace = nullptr;
    // std::cout << "    checking " << m_faces.size() << " faces\n";
    for (Face& face : m_faces) {
        if (face.intersects(ray)) {
            if (closestFace == nullptr || face.getT() < closestFace->getT()) {
                closestFace = &face;
            }
        }
    }
    // if (closestFace != nullptr) {
    //     std::cout << std::hex;
    //     for (Face& face : m_faces) {
    //         std::cout << face.getData()[0] << ' ' << face.getData()[1] << ' ';
    //         std::cout << face.getData()[2] << ' ' << face.getData()[3] << ' ';
    //         std::cout << face.getData()[4] << ' ' << face.getData()[5] << '\n';
    //     }
    //     std::cout << std::dec;
    // }
    return closestFace;
}
