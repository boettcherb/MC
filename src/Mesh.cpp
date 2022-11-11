#include "Mesh.h"
#include "ShaderProgram.h"

#include <glad/glad.h>

Mesh::Mesh() : m_vertexCount{ 0 } {
    glGenVertexArrays(1, &m_vertexArrayID);
    glGenBuffers(1, &m_vertexBufferID);
}

void Mesh::setVertexData(unsigned int size, const void* data) {
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
}

Mesh::~Mesh() {
    glDeleteVertexArrays(1, &m_vertexArrayID);
    glDeleteBuffers(1, &m_vertexBufferID);
}

void Mesh::render(const ShaderProgram* shader) const {
    shader->bind();
    glBindVertexArray(m_vertexArrayID);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}
