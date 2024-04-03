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
m_blocks{ nullptr }, m_numNeighbors{ 0 }, m_updated{ false }, m_rendered{ false } {
    m_neighbors.fill(nullptr);
    if (blockData == nullptr) {
        Block::BlockType* data = new Block::BlockType[BLOCKS_PER_CHUNK];
        generateTerrain(data, 1337);
        m_blocks = new BlockList(data, BLOCKS_PER_CHUNK);
        delete[] data;
    } else {
        m_blocks = new BlockList(blockData, BLOCKS_PER_CHUNK);
    }
    for (int i = 0; i < CHUNK_WIDTH; ++i) {
        for (int k = 0; k < CHUNK_WIDTH; ++k) {
            m_highest_solid_block[i][k] = CHUNK_HEIGHT - 1;
            for (int j = CHUNK_HEIGHT - 1; j >= 0; --j) {
                if (!Block::isTransparent(m_blocks->get(i, j, k))) {
                    m_highest_solid_block[i][k] = (unsigned char) j;
                    break;
                }
            }
        }
    }
}

Chunk::~Chunk() {
    delete m_blocks;
    for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
        m_mesh[i].erase();
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
    return m_blocks->get_all();
}

int Chunk::index(int x, int y, int z) {
    assert(x >= 0 && x < CHUNK_WIDTH);
    assert(y >= 0 && y < CHUNK_HEIGHT);
    assert(z >= 0 && z < CHUNK_WIDTH);
    assert(x * CHUNK_WIDTH * CHUNK_HEIGHT + z * CHUNK_HEIGHT + y < BLOCKS_PER_CHUNK);
    return x * CHUNK_WIDTH * CHUNK_HEIGHT + z * CHUNK_HEIGHT + y;
}

void Chunk::put(int x, int y, int z, Block::BlockType block, bool update_mesh) {
    m_blocks->put(x, y, z, block);
    if (update_mesh) {
        int updateIndex = y / SUBCHUNK_HEIGHT;
        updateMesh(updateIndex);
        if (y != CHUNK_HEIGHT - 1 && y % SUBCHUNK_HEIGHT == 15) updateMesh(updateIndex + 1);
        else if (y != 0 && y % SUBCHUNK_HEIGHT == 0) updateMesh(updateIndex - 1);
        if (x == CHUNK_WIDTH - 1) m_neighbors[PLUS_X]->updateMesh(updateIndex);
        else if (x == 0)          m_neighbors[MINUS_X]->updateMesh(updateIndex);
        if (z == CHUNK_WIDTH - 1) m_neighbors[PLUS_Z]->updateMesh(updateIndex);
        else if (z == 0)          m_neighbors[MINUS_Z]->updateMesh(updateIndex);
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
        return m_blocks->get(x, y, z);
    }
    assert(m_numNeighbors == 4);
    if (x < 0) return m_neighbors[MINUS_X]->get(CHUNK_WIDTH - 1, y, z);
    if (z < 0) return m_neighbors[MINUS_Z]->get(x, y, CHUNK_WIDTH - 1);
    if (x > CHUNK_WIDTH - 1) return m_neighbors[PLUS_X]->get(0, y, z);
    if (z > CHUNK_WIDTH - 1) return m_neighbors[PLUS_Z]->get(x, y, 0);
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
            subChunksRendered += m_mesh[i].render(shader);
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
            updateMesh(i);
        }
        m_rendered = true;
        updated = true;
    }
    else if (m_rendered && m_numNeighbors != 4) {
        // unload this chunk
        for (int i = 0; i < NUM_SUBCHUNKS; ++i) {
            m_mesh[i].erase();
        }
        m_rendered = false;
    }
    return updated;
}

void Chunk::updateMesh(int meshIndex) {
    assert(meshIndex >= 0 && meshIndex < NUM_SUBCHUNKS);
    m_mesh[meshIndex].erase();
    // lim is the maximum possible number of attributes that can be rendered per
    // subchunk (3 attribs per vertex, 6 vertices per face, 6 faces per block,
    // 4096 blocks per subchunk). This is an estimate because some blocks do not
    // have 6 faces (ex: plants only have 4). Most subchunks will not be
    // anywhere near this number, so divide by 8 to save memory.
    constexpr unsigned int lim = ATTRIBS_PER_FACE * 6 * BLOCKS_PER_SUBCHUNK / 8;
    VertexAttribType* data = new VertexAttribType[lim];
    unsigned int size = getVertexData(data, meshIndex);
    assert(size <= lim * 4);
    m_mesh[meshIndex].generate(size, data, true, m_posX, m_posZ);
    delete[] data;
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

// update this function!
// 1. Store a 16x16 array holding the highest non-air block in the chunk. Then, only
//    check blocks below that block. Skip solid blocks because most should be solid.
// 2. call blocks->get_all() at the start of the function and split up the for loops
//    into inner blocks (prevents bounds checking) and outer blocks (we already know
//    where the out of bounds blocks will be)
unsigned int Chunk::getVertexData(VertexAttribType* data, int meshIndex) const {
    VertexAttribType* start = data; // record the current byte address
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            for (int y = meshIndex * SUBCHUNK_HEIGHT; y < (meshIndex + 1) * SUBCHUNK_HEIGHT; ++y) {
                Block::BlockType currentBlock = get(x, y, z);
                assert(currentBlock != Block::BlockType::NO_BLOCK);
                if (currentBlock == Block::BlockType::AIR)
                    continue;
                bool dirHasBlock[(int) NUM_DIRECTIONS] = {
                    !Block::isTransparent(get(x + 1, y, z)),
                    !Block::isTransparent(get(x - 1, y, z)),
                    !Block::isTransparent(get(x, y, z + 1)),
                    !Block::isTransparent(get(x, y, z - 1)),
                    !Block::isTransparent(get(x, y + 1, z)),
                    !Block::isTransparent(get(x, y - 1, z)),
                };
                data += Block::getBlockData(currentBlock, x, y, z, data, dirHasBlock);
            }
        }
    }
    // return the number of bytes that were initialized
    return (unsigned int) ((data - start) * sizeof(VertexAttribType));
}

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
        if (m_mesh[sc_index].intersects(ray, i)) {
            if (!foundIntersection || i.t < isect.t) {
                foundIntersection = true;
                isect = i;
            }
        }
    }
    return foundIntersection;
}
