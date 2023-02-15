#include "Chunk.h"
#include "BlockInfo.h"
#include "Shader.h"
#include "Mesh.h"
#include "Face.h"
#include <sglm/sglm.h>
#include <FastNoise/FastNoise.h>
#include <new>
#include <cassert>

Chunk::Chunk(int x, int z) : m_posX{ x }, m_posZ{ z } {
    m_neighbors[0] = m_neighbors[1] = m_neighbors[2] = m_neighbors[3] = nullptr;
    m_numNeighbors = 0;
    generateTerrain();
}

void Chunk::updateMesh() {
    for (int i = 0; i < NUM_MESHES; ++i) {
        m_mesh[i].erase();
        unsigned int* data = new unsigned int[VERTICES_PER_MESH];
        unsigned int size = getVertexData(data, i);
        m_mesh[i].generate(size, data, true);
        delete[] data;
    }
}

void Chunk::generateTerrain() {
    FastNoise noise;
    for (int X = 0; X < CHUNK_LENGTH; ++X) {
        for (int Z = 0; Z < CHUNK_WIDTH; ++Z) {
            float noiseX = (float) X + CHUNK_LENGTH * m_posX;
            float noiseZ = (float) Z + CHUNK_WIDTH * m_posZ;
            int groundHeight = (int) (50.0 + (noise.GetSimplexFractal(noiseX, noiseZ) + 1.0) / 2.0 * 30.0);
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
        m_mesh[i].erase();
    }
    if (m_neighbors[PLUS_X] != nullptr) m_neighbors[PLUS_X]->removeNeighbor(MINUS_X);
    if (m_neighbors[MINUS_X] != nullptr) m_neighbors[MINUS_X]->removeNeighbor(PLUS_X);
    if (m_neighbors[PLUS_Z] != nullptr) m_neighbors[PLUS_Z]->removeNeighbor(MINUS_Z);
    if (m_neighbors[MINUS_Z] != nullptr) m_neighbors[MINUS_Z]->removeNeighbor(PLUS_Z);
}

void Chunk::put(int x, int y, int z, Block::BlockType block) {
    assert(x >= 0 && x < CHUNK_LENGTH);
    assert(y >= 0 && y < CHUNK_HEIGHT);
    assert(z >= 0 && z < CHUNK_WIDTH);
    m_blockArray[x][y][z] = block;
}

Block::BlockType Chunk::get(int x, int y, int z) const {
    assert(x >= -1 && x <= CHUNK_LENGTH);
    assert(y >= -1 && y <= CHUNK_HEIGHT);
    assert(z >= -1 && z <= CHUNK_WIDTH);
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
    return Block::BlockType::NO_BLOCK;
}

// the view and projection matrices must be set before this function is called.
void Chunk::render(Shader* shader) {
    if (m_numNeighbors != 4) {
        return;
    }
    // send the model matrix to the shaders
    sglm::vec3 translation = { (float) m_posX * CHUNK_LENGTH, 0.0f, (float) m_posZ * CHUNK_WIDTH };
    shader->addUniformMat4f("u0_model", sglm::translate(translation));

    // render each mesh
    for (int i = 0; i < NUM_MESHES; ++i) {
        m_mesh[i].render(shader);
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
            m_mesh[i].erase();
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
                Block::BlockType currentBlock = get(x, y, z);
                assert(currentBlock != Block::BlockType::NO_BLOCK);
                if (currentBlock == Block::BlockType::AIR) {
                    continue;
                }
                // check each of the six sides to see if this block is adjacent to a transparent block
                // if so, add its vertex data to the data array
                if (Block::isTransparent(get(x + 1, y, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::PLUS_X));
                    data += UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x - 1, y, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::MINUS_X));
                    data += UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y + 1, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::PLUS_Y));
                    data += UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y - 1, z))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::MINUS_Y));
                    data += UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y, z + 1))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::PLUS_Z));
                    data += UINTS_PER_FACE;
                }
                if (Block::isTransparent(get(x, y, z - 1))) {
                    setBlockFaceData(data, x, y, z, Block::getData(currentBlock, Block::BlockFace::MINUS_Z));
                    data += UINTS_PER_FACE;
                }
            }
        }
    }

    // return the number of bytes that were initialized
    return (unsigned int) (data - start) * sizeof(unsigned int);
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
    for (unsigned int vertex = 0; vertex < VERTICES_PER_FACE; ++vertex) {
        // x pos takes bits 23-27, y takes bits 15-22, z takes bits 10-14 (from the right)
        // add the relative x, y, and z positions of the block in the chunk
        data[vertex] = blockData[vertex] + (x << 23) + (y << 15) + (z << 10);
    }
}

Face* Chunk::findViewRayIntersection(const Ray& ray) {
    float x = ray.getPosition().x;
    float y = ray.getPosition().y;
    float z = ray.getPosition().z;
    int cx = m_posX * CHUNK_LENGTH;
    int cz = m_posZ * CHUNK_WIDTH;
    if (x + PLAYER_REACH < cx || x - PLAYER_REACH > cx + CHUNK_LENGTH) {
        return nullptr;
    }
    if (z + PLAYER_REACH < cz || z - PLAYER_REACH > cz + CHUNK_WIDTH) {
        return nullptr;
    }
    Face* bestFace = nullptr;
    for (int i = 0; i < NUM_MESHES; ++i) {
        int my = i * MESH_HEIGHT;
        if (y + PLAYER_REACH < my || y - PLAYER_REACH > my + MESH_HEIGHT) {
            continue;
        }
        Face* face = m_mesh[i].intersects(ray);
        if (face != nullptr) {
            if (bestFace == nullptr || face->getT() < bestFace->getT()) {
                bestFace = face;
            }
        }
    }
    return bestFace;
}
