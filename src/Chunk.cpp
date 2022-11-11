#include "Chunk.h"
#include "BlockInfo.h"
#include "ShaderProgram.h"
#include "Mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <FastNoise/FastNoise.h>

#include <new>

Chunk::Chunk(float x, float z, ShaderProgram* shader) : m_posX{ x }, m_posZ{ z }, m_shader{ shader } {
    m_blocks = new Blocks();
    m_neighbors[0] = m_neighbors[1] = m_neighbors[2] = m_neighbors[3] = nullptr;
    generateTerrain();
}

void Chunk::updateMesh() {
    if (m_mesh != nullptr) {
        delete m_mesh;
    }
    m_mesh = new Mesh();
    unsigned int* data = new unsigned int[BLOCKS_PER_CHUNK * Block::VERTICES_PER_BLOCK];
    unsigned int size = getVertexData(data);
    m_mesh->setVertexData(size, data);
    delete[] data;
}

void Chunk::generateTerrain() {
    FastNoise noise;
    for (int X = 0; X < CHUNK_LENGTH; ++X) {
        for (int Z = 0; Z < CHUNK_WIDTH; ++Z) {
            float noiseX = X + CHUNK_LENGTH * m_posX;
            float noiseZ = Z + CHUNK_WIDTH * m_posZ;
            int groundHeight = static_cast<int>(50.0 + (noise.GetSimplexFractal(noiseX, noiseZ) + 1.0) / 2.0 * 30.0);
            for (int Y = 0; Y <= groundHeight - 4; ++Y) {
                put(X, Y, Z, Block::BlockType::STONE);
            }
            put(X, groundHeight - 3, Z, Block::BlockType::DIRT);
            put(X, groundHeight - 2, Z, Block::BlockType::DIRT);
            put(X, groundHeight - 1, Z, Block::BlockType::DIRT);
            put(X, groundHeight, Z, Block::BlockType::GRASS);
            for (int Y = groundHeight + 1; Y < CHUNK_HEIGHT; ++Y) {
                put(X, Y, Z, Block::BlockType::AIR);
            }
        }
    }
}

Chunk::~Chunk() {
    delete m_mesh;
    delete m_blocks;
}

void Chunk::put(int x, int y, int z, Block::BlockType block) {
    m_blocks->m_blockArray[x][y][z] = block;
}

Block::BlockType Chunk::get(int x, int y, int z) const {
    if (x >= 0 && y >= 0 && z >= 0 && x < CHUNK_LENGTH && y < CHUNK_HEIGHT && z < CHUNK_WIDTH) {
        return m_blocks->m_blockArray[x][y][z];
    }
    if (x > CHUNK_LENGTH - 1 && m_neighbors[PLUS_X] != nullptr) {
        return m_neighbors[PLUS_X]->get(0, y, z);
    }
    if (x < 0 && m_neighbors[MINUS_X] != nullptr) {
        return m_neighbors[MINUS_X]->get(CHUNK_LENGTH - 1, y, z);
    }
    if (z > CHUNK_WIDTH - 1 && m_neighbors[PLUS_Z] != nullptr) {
        return m_neighbors[PLUS_Z]->get(x, y, 0);
    }
    if (z < 0 && m_neighbors[MINUS_Z] != nullptr) {
        return m_neighbors[MINUS_Z]->get(x, y, CHUNK_WIDTH - 1);
    }
    // default: no block
    return Block::BlockType::AIR;
}

void Chunk::render(glm::mat4 viewMatrix, float zoom, float scrRatio) {
    // send the MVP matrices to the shaders
    glm::vec3 translation(m_posX * CHUNK_LENGTH, 0.0f, m_posZ * CHUNK_WIDTH);
    m_shader->addUniformMat4f("u_model", glm::translate(glm::mat4(1.0f), translation));
    m_shader->addUniformMat4f("u_view", viewMatrix);
    glm::mat4 projection = glm::perspective(glm::radians(zoom), scrRatio, 0.1f, 300.0f);
    m_shader->addUniformMat4f("u_projection", projection);
    m_mesh->render(m_shader);
}

void Chunk::addNeighbor(Chunk* chunk, Direction direction) {
    m_neighbors[direction] = chunk;
}

unsigned int Chunk::getVertexData(unsigned int* data) const {
    // record the current byte address
    unsigned int* start = data;
    for (int x = 0; x < CHUNK_LENGTH; ++x) {
        for (int y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int z = 0; z < CHUNK_WIDTH; ++z) {
                // skip if this block is air
                Block::BlockType currentBlock = get(x, y, z);
                if (currentBlock == Block::BlockType::AIR) {
                    continue;
                }
                // check each of the six sides to see if this block is adjacent to a transparent block
                if (Block::isTransparent(get(x + 1, y, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::PLUS_X));
                    data += Block::UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x - 1, y, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::MINUS_X));
                    data += Block::UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y + 1, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::PLUS_Y));
                    data += Block::UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y - 1, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::MINUS_Y));
                    data += Block::UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y, z + 1))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::PLUS_Z));
                    data += Block::UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y, z - 1))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::MINUS_Z));
                    data += Block::UINTS_PER_FACE;
                }
            }
        }
    }

    // return the number of bytes that were initialized
    return static_cast<unsigned int>(data - start) * sizeof(unsigned int);
}

inline void Chunk::setBlockFaceData(unsigned int* data, int x, int y, int z, const unsigned int* blockData) const {
    for (unsigned int vertex = 0; vertex < Block::VERTICES_PER_FACE; ++vertex) {
        // x pos takes bits 23-27, y takes bits 15-22, z takes bits 10-14 (from the right)
        // add the relative x, y, and z positions of the block in the chunk
        data[vertex] = blockData[vertex] + (x << 23) + (y << 15) + (z << 10);
    }
}
