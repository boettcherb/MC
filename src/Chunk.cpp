#include "Chunk.h"
#include "BlockInfo.h"
#include "Shader.h"
#include "Mesh.h"
#include "Face.h"
#include <sglm/sglm.h>
#include <new>
#include <algorithm>
#include <cassert>

Chunk::Chunk(int x, int z, const Block::BlockType* blockData): m_posX{ x }, m_posZ{ z },
    m_numNeighbors{0}, m_updated{false}, m_rendered{false} {
    m_neighbors.fill(nullptr);
    if (blockData == nullptr) {
        Block::BlockType* data = new Block::BlockType[BLOCKS_PER_CHUNK];
        generateTerrain(data, 1337);
        for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
            m_subchunks[i] = new Subchunk(m_posX, i, m_posZ, data + i * BLOCKS_PER_SUBCHUNK);
        }
        delete[] data;
    } else {
        for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
            m_subchunks[i] = new Subchunk(m_posX, i, m_posZ, blockData + i * BLOCKS_PER_SUBCHUNK);
        }
    }
    // for (int i = 0; i < CHUNK_WIDTH; ++i) {
    //     for (int k = 0; k < CHUNK_WIDTH; ++k) {
    //         m_highest_solid_block[i][k] = CHUNK_HEIGHT - 1;
    //         for (int j = CHUNK_HEIGHT - 1; j >= 0; --j) {
    //             if (!Block::isTransparent(m_blocks->get(i, j, k))) {
    //                 m_highest_solid_block[i][k] = (unsigned char) j;
    //                 break;
    //             }
    //         }
    //     }
    // }
}

Chunk::~Chunk() {
    for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
        delete m_subchunks[i];
    }
    if (m_neighbors[PLUS_X] != nullptr) m_neighbors[PLUS_X]->removeNeighbor(MINUS_X);
    if (m_neighbors[MINUS_X] != nullptr) m_neighbors[MINUS_X]->removeNeighbor(PLUS_X);
    if (m_neighbors[PLUS_Z] != nullptr) m_neighbors[PLUS_Z]->removeNeighbor(MINUS_Z);
    if (m_neighbors[MINUS_Z] != nullptr) m_neighbors[MINUS_Z]->removeNeighbor(PLUS_Z);
}

bool Chunk::wasUpdated() const {
    return m_updated;
}

// The caller is responsible for freeing the returned array.
const void* Chunk::getBlockData() const {
    Block::BlockType* data = new Block::BlockType[BLOCKS_PER_CHUNK];
    for (int i = 0; i < 8; ++i) {
        Block::BlockType* subchunk_data = m_subchunks[i]->m_blocks.get_all();
        std::copy(subchunk_data, subchunk_data + BLOCKS_PER_SUBCHUNK, data + i * BLOCKS_PER_SUBCHUNK);
        delete[] subchunk_data;
    }
    return data;
}

int Chunk::subchunk_index(int x, int y, int z) {
    assert(x >= 0 && x < CHUNK_WIDTH);
    assert(y >= 0 && y < SUBCHUNK_HEIGHT);
    assert(z >= 0 && z < CHUNK_WIDTH);
    assert(x * CHUNK_WIDTH * SUBCHUNK_HEIGHT + z * SUBCHUNK_HEIGHT + y < BLOCKS_PER_SUBCHUNK);
    return x * CHUNK_WIDTH * SUBCHUNK_HEIGHT + z * SUBCHUNK_HEIGHT + y;
}

int Chunk::chunk_index(int x, int y, int z) {
    int index = subchunk_index(x, y % SUBCHUNK_HEIGHT, z);
    return index + BLOCKS_PER_SUBCHUNK * (y / SUBCHUNK_HEIGHT);
}

void Chunk::put(int x, int y, int z, Block::BlockType block, bool update_mesh) {
    m_subchunks[y / SUBCHUNK_HEIGHT]->m_blocks.put(x, y % SUBCHUNK_HEIGHT, z, block);
    if (update_mesh) {
        int updateIndex = y / SUBCHUNK_HEIGHT;
        m_subchunks[updateIndex]->updateMesh(this);
        if (y != CHUNK_HEIGHT - 1 && y % SUBCHUNK_HEIGHT == SUBCHUNK_HEIGHT - 1) {
            m_subchunks[updateIndex + 1]->updateMesh(this);
        }
        else if (y != 0 && y % SUBCHUNK_HEIGHT == 0) {
            m_subchunks[updateIndex - 1]->updateMesh(this);
        }
        if (x == CHUNK_WIDTH - 1) m_neighbors[PLUS_X]->m_subchunks[updateIndex]->updateMesh(m_neighbors[PLUS_X]);
        else if (x == 0)          m_neighbors[MINUS_X]->m_subchunks[updateIndex]->updateMesh(m_neighbors[MINUS_X]);
        if (z == CHUNK_WIDTH - 1) m_neighbors[PLUS_Z]->m_subchunks[updateIndex]->updateMesh(m_neighbors[PLUS_Z]);
        else if (z == 0)          m_neighbors[MINUS_Z]->m_subchunks[updateIndex]->updateMesh(m_neighbors[MINUS_Z]);
    }
    m_updated = true;
}

// x, y, and z could be 1 outside the range because in Chunk::getVertexData()
// we check each block's surrounding blocks to see if they are transparent.
Block::BlockType Chunk::get(int x, int y, int z) const {
    assert(x >= -1 && x <= CHUNK_WIDTH);
    assert(y >= -1 && y <= CHUNK_HEIGHT);
    assert(z >= -1 && z <= CHUNK_WIDTH);
    if (y == -1 || y == CHUNK_HEIGHT) {
        return Block::BlockType::NO_BLOCK;
    }
    if (x >= 0 && z >= 0 && x < CHUNK_WIDTH && z < CHUNK_WIDTH) {
        return m_subchunks[y / SUBCHUNK_HEIGHT]->m_blocks.get(x, y % SUBCHUNK_HEIGHT, z);
    }
    assert(m_numNeighbors == 4);
    if (x < 0) return m_neighbors[MINUS_X]->get(CHUNK_WIDTH - 1, y, z);
    if (z < 0) return m_neighbors[MINUS_Z]->get(x, y, CHUNK_WIDTH - 1);
    if (x >= CHUNK_WIDTH) return m_neighbors[PLUS_X]->get(0, y, z);
    if (z >= CHUNK_WIDTH) return m_neighbors[PLUS_Z]->get(x, y, 0);
    assert(0);
    return Block::BlockType::NO_BLOCK;
}

// the view and projection matrices must be set before this function is called.
int Chunk::render(Shader* shader, const sglm::frustum& frustum) {
    int subChunksRendered = 0;

    // send the model matrix to the shaders
    float cx = (float) (m_posX * CHUNK_WIDTH);
    float cz = (float) (m_posZ * CHUNK_WIDTH);
    shader->addUniformMat4f("u0_model", sglm::translate({ cx, 0.0f, cz }));

    // render each mesh
    for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
        float cx2 = cx + CHUNK_WIDTH / 2.0f;
        float cz2 = cz + CHUNK_WIDTH / 2.0f;
        float cy2 = i * SUBCHUNK_HEIGHT + SUBCHUNK_HEIGHT / 2.0f;
        if (frustum.contains({ cx2, cy2, cz2 }, SUB_CHUNK_RADIUS)) {
            subChunksRendered += m_subchunks[i]->m_mesh.render(shader);
        }
    }
    return subChunksRendered;
}

// called (basically) every frame by World::update()
bool Chunk::update() {
    bool updated = false;
    if (!m_rendered && m_numNeighbors == 4) {
        // render this chunk
        for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
            m_subchunks[i]->updateMesh(this);
        }
        m_rendered = true;
        updated = true;
    }
    else if (m_rendered && m_numNeighbors != 4) {
        // unload this chunk
        for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
            m_subchunks[i]->m_mesh.erase();
        }
        m_rendered = false;
    }
    return updated;
}

void Chunk::addNeighbor(Chunk* chunk, Direction direction) {
    assert(m_neighbors[direction] == nullptr);
    m_neighbors[direction] = chunk;
    ++m_numNeighbors;
    assert(m_numNeighbors >= 0 && m_numNeighbors <= 4);
}

void Chunk::removeNeighbor(Direction direction) {
    assert(m_neighbors[direction] != nullptr);
    m_neighbors[direction] = nullptr;
    --m_numNeighbors;
    assert(m_numNeighbors >= 0 && m_numNeighbors < 4);
}

std::pair<std::pair<int, int>, Chunk*> Chunk::getNeighbor(int index) const {
    int x = m_posX + (index == PLUS_X) - (index == MINUS_X);
    int z = m_posZ + (index == PLUS_Z) - (index == MINUS_Z);
    return { { x, z }, m_neighbors[index] };
}

int Chunk::getNumNeighbors() const {
    return m_numNeighbors;
}

// int Chunk::getHighestBlock(int x, int z) const {
//     return m_highest_solid_block[x][z];
// }

bool Chunk::intersects(const sglm::ray& ray, Face::Intersection& isect) {
    auto [x, y, z] = ray.pos;
    int cx = m_posX * CHUNK_WIDTH;
    int cz = m_posZ * CHUNK_WIDTH;
    if (x + ray.length < cx || x - ray.length > cx + CHUNK_WIDTH)
        return false;
    if (z + ray.length < cz || z - ray.length > cz + CHUNK_WIDTH)
        return false;
    bool foundIntersection = false;
    for (int sc_index = 0; sc_index < NUM_SUBCHUNKS; ++sc_index) {
        int sc_y = sc_index * SUBCHUNK_HEIGHT;
        if (y + ray.length < sc_y || y - ray.length > sc_y + SUBCHUNK_HEIGHT) {
            continue;
        }
        Face::Intersection i;
        if (m_subchunks[sc_index]->m_mesh.intersects(ray, i)) {
            if (!foundIntersection || i.t < isect.t) {
                foundIntersection = true;
                isect = i;
            }
        }
    }
    return foundIntersection;
}
