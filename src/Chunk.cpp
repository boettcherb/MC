#include "Chunk.h"
#include "Block.h"
#include "Shader.h"
#include "Mesh.h"
#include "Face.h"
#include <sglm/sglm.h>
#include <new>
#include <algorithm>
#include <cassert>

Chunk::Chunk(int x, int z) : m_X{ x }, m_Z{ z }, m_numNeighbors{ 0 },
m_toDelete{ false }, m_updated{ false }, m_status{ Status::EMPTY } {
    m_neighbors.fill(nullptr);
    for (int y = 0; y < NUM_SUBCHUNKS; ++y) {
        m_subchunks[y] = new Subchunk(y);
    }
}

Chunk::~Chunk() {
    for (Subchunk* subchunk : m_subchunks) {
        delete subchunk;
    }
    if (m_neighbors[PLUS_X] != nullptr) m_neighbors[PLUS_X]->removeNeighbor(MINUS_X);
    if (m_neighbors[MINUS_X] != nullptr) m_neighbors[MINUS_X]->removeNeighbor(PLUS_X);
    if (m_neighbors[PLUS_Z] != nullptr) m_neighbors[PLUS_Z]->removeNeighbor(MINUS_Z);
    if (m_neighbors[MINUS_Z] != nullptr) m_neighbors[MINUS_Z]->removeNeighbor(PLUS_Z);
}

bool Chunk::wasUpdated() const { return m_updated; }
void Chunk::updateHandled() { m_updated = false; }
Chunk::Status Chunk::getStatus() const { return m_status; }
void Chunk::setLoading() { m_status = Status::LOADING; }
void Chunk::setToDelete() { m_toDelete = true; }

// The caller is responsible for freeing the returned array.
const void* Chunk::getBlockData() const {
    Block::BlockType* data = new Block::BlockType[BLOCKS_PER_CHUNK];
    for (int y = 0; y < NUM_SUBCHUNKS; ++y) {
        Block::BlockType* subchunk_data = m_subchunks[y]->m_blocks.get_all();
        std::copy(subchunk_data, subchunk_data + BLOCKS_PER_SUBCHUNK, data + y * BLOCKS_PER_SUBCHUNK);
        delete[] subchunk_data;
    }
    return data;
}

void Chunk::put(int x, int y, int z, Block::BlockType block) {
    assert(m_status >= Status::TERRAIN);
    assert(x >= 0 && x < CHUNK_WIDTH);
    assert(y >= 0 && y < CHUNK_HEIGHT);
    assert(z >= 0 && z < CHUNK_WIDTH);
    assert(Block::isReal(block));
    int subchunk = y / SUBCHUNK_HEIGHT;
    m_subchunks[subchunk]->m_blocks.put(x, y % SUBCHUNK_HEIGHT, z, block);
    m_subchunks[subchunk]->updateMesh(this);

    // if we're updating a block on the border of the subchunk, we also
    // have to update the neighboring subchunk
    if (y != CHUNK_HEIGHT - 1 && y % SUBCHUNK_HEIGHT == SUBCHUNK_HEIGHT - 1)
        m_subchunks[subchunk + 1]->updateMesh(this);
    else if (y != 0 && y % SUBCHUNK_HEIGHT == 0)
        m_subchunks[subchunk - 1]->updateMesh(this);
    assert(m_numNeighbors == 4);
    if (x == CHUNK_WIDTH - 1)
        m_neighbors[PLUS_X]->m_subchunks[subchunk]->updateMesh(m_neighbors[PLUS_X]);
    else if (x == 0)
        m_neighbors[MINUS_X]->m_subchunks[subchunk]->updateMesh(m_neighbors[MINUS_X]);
    if (z == CHUNK_WIDTH - 1)
        m_neighbors[PLUS_Z]->m_subchunks[subchunk]->updateMesh(m_neighbors[PLUS_Z]);
    else if (z == 0)
        m_neighbors[MINUS_Z]->m_subchunks[subchunk]->updateMesh(m_neighbors[MINUS_Z]);

    m_updated = true;
}

// x, y, and z could be 1 outside the range because in Chunk::getVertexData()
// we check each block's surrounding blocks to see if they are transparent.
Block::BlockType Chunk::get(int x, int y, int z) const {
    assert(m_status >= Status::TERRAIN);
    assert(x >= -1 && x <= CHUNK_WIDTH);
    assert(y >= -1 && y <= CHUNK_HEIGHT);
    assert(z >= -1 && z <= CHUNK_WIDTH);
    if (y == -1 || y == CHUNK_HEIGHT)
        return Block::BlockType::NO_BLOCK;
    if (x >= 0 && z >= 0 && x < CHUNK_WIDTH && z < CHUNK_WIDTH)
        return m_subchunks[y / SUBCHUNK_HEIGHT]->m_blocks.get(x, y % SUBCHUNK_HEIGHT, z);
    assert(m_numNeighbors == 4);
    if (x < 0)
        return m_neighbors[MINUS_X]->get(CHUNK_WIDTH - 1, y, z);
    else if (z < 0)
        return m_neighbors[MINUS_Z]->get(x, y, CHUNK_WIDTH - 1);
    else if (x >= CHUNK_WIDTH)
        return m_neighbors[PLUS_X]->get(0, y, z);
    else
        return m_neighbors[PLUS_Z]->get(x, y, 0);
}

// the view and projection matrices must be set before this function is called.
int Chunk::render(Shader* shader, const sglm::frustum& frustum) {
    int subChunksRendered = 0;
    float cx = (float) (m_X * CHUNK_WIDTH);
    float cz = (float) (m_Z * CHUNK_WIDTH);
    for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
        float cy = (float) (i * SUBCHUNK_HEIGHT);
        float ox = CHUNK_WIDTH / 2.0f;
        float oy = SUBCHUNK_HEIGHT / 2.0f;
        float oz = CHUNK_WIDTH / 2.0f;
        if (frustum.contains({ cx + ox, cy + oy, cz + oz }, SUB_CHUNK_RADIUS)) {
            shader->addUniformMat4f("u0_model", sglm::translate({ cx, cy, cz }));
            subChunksRendered += m_subchunks[i]->m_mesh.render(shader);
        }
    }
    return subChunksRendered;
}

// called (basically) every frame by World::update()
bool Chunk::update() {
    bool rendered = false;
    if (m_toDelete) {
        deleteBlockData();
        m_toDelete = false;
    }
    else if (m_status == Status::TERRAIN && m_numNeighbors == 4) {
        // render this chunk
        for (Subchunk* subchunk : m_subchunks) {
            subchunk->updateMesh(this);
        }
        m_status = Status::FULL;
        rendered = true;
    }
    else if (m_status == Status::FULL && m_numNeighbors != 4) {
        // unload this chunk
        for (Subchunk* subchunk : m_subchunks) {
            subchunk->m_mesh.erase();
        }
        m_status = Status::TERRAIN;
    }
    return rendered;
}

void Chunk::addBlockData(const Block::BlockType* blockData) {
    assert(m_status == Status::LOADING);
    for (int y = 0; y < NUM_SUBCHUNKS; ++y) {
        m_subchunks[y]->m_blocks.create(blockData + y * BLOCKS_PER_SUBCHUNK, BLOCKS_PER_SUBCHUNK);
    }
    for (int neighbor = 0; neighbor < 4; ++neighbor) {
        Chunk* n = m_neighbors[neighbor];
        assert(n->m_neighbors[neighbor + (neighbor % 2 ? -1 : 1)] == this);
        ++n->m_numNeighbors;
        assert(n->m_numNeighbors <= 4);
    }
    m_status = Status::TERRAIN;
}

void Chunk::deleteBlockData() {
    assert(m_status == Status::TERRAIN || m_status == Status::FULL);
    for (Subchunk* subchunk : m_subchunks) {
        subchunk->m_blocks.deleteAll();
        subchunk->m_mesh.erase();
    }
    for (int neighbor = 0; neighbor < 4; ++neighbor) {
        Chunk* n = m_neighbors[neighbor];
        if (n != nullptr) {
            assert(n->m_neighbors[neighbor + (neighbor % 2 ? -1 : 1)] == this);
            --n->m_numNeighbors;
            assert(n->m_numNeighbors >= 0);
        }
    }
    m_status = Status::STRUCTURES;
}

void Chunk::addNeighbor(Chunk* chunk, Direction direction) {
    assert(m_neighbors[direction] == nullptr);
    m_neighbors[direction] = chunk;
    assert(m_numNeighbors < 4);
    assert(m_status < Status::FULL);
}

void Chunk::removeNeighbor(Direction direction) {
    assert(m_neighbors[direction] != nullptr);
    assert(m_numNeighbors < 4);
    assert(m_status < Status::FULL);
    m_neighbors[direction] = nullptr;
}

std::pair<std::pair<int, int>, Chunk*> Chunk::getNeighbor(int index) const {
    int x = m_X + (index == PLUS_X) - (index == MINUS_X);
    int z = m_Z + (index == PLUS_Z) - (index == MINUS_Z);
    return { { x, z }, m_neighbors[index] };
}

bool Chunk::intersects(const sglm::ray& ray, Face::Intersection& isect) {
    auto [x, y, z] = ray.pos;
    int cx = m_X * CHUNK_WIDTH;
    int cz = m_Z * CHUNK_WIDTH;
    if (x + ray.length < cx || x - ray.length > cx + CHUNK_WIDTH)
        return false;
    if (z + ray.length < cz || z - ray.length > cz + CHUNK_WIDTH)
        return false;
    bool foundIntersection = false;
    for (int subchunk = 0; subchunk < NUM_SUBCHUNKS; ++subchunk) {
        int sc_y = subchunk * SUBCHUNK_HEIGHT;
        if (y + ray.length < sc_y || y - ray.length > sc_y + SUBCHUNK_HEIGHT) {
            continue;
        }
        Face::Intersection i;
        if (m_subchunks[subchunk]->m_mesh.intersects(ray, i)) {
            if (!foundIntersection || i.t < isect.t) {
                foundIntersection = true;
                isect = i;
                isect.cy = subchunk;
            }
        }
    }
    return foundIntersection;
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
