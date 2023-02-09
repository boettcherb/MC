#include "Chunk.h"
#include "BlockInfo.h"
#include "Shader.h"
#include "Mesh.h"
#include <math/sglm.h>
#include <FastNoise/FastNoise.h>
#include <new>
#include <cassert>

Chunk::Chunk(float x, float z, Shader* shader) : m_posX{ x }, m_posZ{ z }, m_shader{ shader } {
    m_neighbors[0] = m_neighbors[1] = m_neighbors[2] = m_neighbors[3] = nullptr;
    m_numNeighbors = 0;
    for (int i = 0; i < NUM_MESHES; ++i) {
        m_mesh[i] = nullptr;
    }
    generateTerrain();
}

void Chunk::updateMesh() {
    for (int i = 0; i < NUM_MESHES; ++i) {
        if (m_mesh[i] != nullptr) {
            delete m_mesh[i];
        }
        m_mesh[i] = new Mesh();
        unsigned int* data = new unsigned int[BLOCKS_PER_MESH * Block::VERTICES_PER_BLOCK];
        unsigned int size = getVertexData(data, i);
        m_mesh[i]->setVertexData(size, data);
        delete[] data;
    }
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
    for (int i = 0; i < NUM_MESHES; ++i) {
        delete m_mesh[i];
    }
}

void Chunk::put(int x, int y, int z, Block::BlockType block) {
    m_blockArray[x][y][z] = block;
}

Block::BlockType Chunk::get(int x, int y, int z) const {
    if (x >= 0 && y >= 0 && z >= 0 && x < CHUNK_LENGTH && y < CHUNK_HEIGHT && z < CHUNK_WIDTH) {
        return m_blockArray[x][y][z];
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

void Chunk::render(sglm::mat4 viewMatrix, float zoom, float scrRatio) {
    if (m_numNeighbors != 4) {
        return;
    }
    // send the MVP matrices to the shaders
    sglm::vec3 translation = { m_posX * CHUNK_LENGTH, 0.0f, m_posZ * CHUNK_WIDTH };
    m_shader->addUniformMat4f("u0_model", sglm::translate(translation));
    m_shader->addUniformMat4f("u1_view", viewMatrix);
    sglm::mat4 projection = sglm::perspective(sglm::radians(zoom), scrRatio, 0.1f, 300.0f);
    m_shader->addUniformMat4f("u2_projection", projection);

    // render each mesh
    for (int i = 0; i < NUM_MESHES; ++i) {
        if (m_mesh[i]->getVertexCount() > 0) {
            m_mesh[i]->render(m_shader);
        }
    }
}

void Chunk::addNeighbor(Chunk* chunk, Direction direction) {
    assert(m_neighbors[direction] == nullptr);
    m_neighbors[direction] = chunk;
    ++m_numNeighbors;
    assert(m_numNeighbors <= 4);

    // all 4 neighboring chunks are loaded so we can now render this chunk
    if (m_numNeighbors == 4) {
        updateMesh();
    }
}

void Chunk::removeNeighbor(Direction direction) {
    assert(m_neighbors[direction] != nullptr);
    m_neighbors[direction] = nullptr;

    // one of this chunk's neighbors was un-loaded
    // so we can no longer render this chunk
    if (m_numNeighbors == 4) {
        for (int i = 0; i < NUM_MESHES; ++i) {
            delete m_mesh[i];
        }
    }
    --m_numNeighbors;
    assert(m_numNeighbors >= 0);
}

unsigned int Chunk::getVertexData(unsigned int* data, int meshIndex) const {
    unsigned int* start = data; // record the current byte address
    for (int x = 0; x < CHUNK_LENGTH; ++x) {
        for (int y = meshIndex * 16; y < meshIndex * 16 + 16; ++y) {
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

//
// A vertex is represented by 1 32-bit unsigned integer:
// x position (5 bits): 1111100000000000000000000000
// y position (8 bits): 0000011111111000000000000000
// z position (5 bits): 0000000000000111110000000000
// x texcoord (5 bits): 0000000000000000001111100000
// y texcoord (5 bits): 0000000000000000000000011111
//
// The x and z positions are values from 0 to 16 and the y position is a value
// from 0 to 128. These represent the position of the vertex within a chunk.
//
// The texture coordinates also range from 0 to 16. The vertex shader divides
// these values by 16 and the results (floats from 0 to 1) determine where in
// the texture to sample from. (0, 0) is bottom left and (1, 1) is top right.
//

inline void Chunk::setBlockFaceData(unsigned int* data, int x, int y, int z, const unsigned int* blockData) const {
    for (unsigned int vertex = 0; vertex < Block::VERTICES_PER_FACE; ++vertex) {
        // x pos takes bits 23-27, y takes bits 15-22, z takes bits 10-14 (from the right)
        // add the relative x, y, and z positions of the block in the chunk
        data[vertex] = blockData[vertex] + (x << 23) + (y << 15) + (z << 10);
    }
}
