#include "Chunk.h"
#include "BlockInfo.h"
#include "Constants.h"

#include <cassert>

Chunk::Subchunk::Subchunk(int x, int y, int z, const Block::BlockType* data) : m_X{ x },
    m_Y{ y }, m_Z{ z }, m_blocks { BlockList(data, BLOCKS_PER_SUBCHUNK) } {
    m_height_centered = SUBCHUNK_HEIGHT * y + SUBCHUNK_HEIGHT / 2;
    // mm_mesh_size = -1;
}

void Chunk::Subchunk::updateMesh(const Chunk* this_chunk) {
    m_mesh.erase();
    // TODO: fix this
    // unsigned int lim = mm_mesh_size == -1 ?
    //     BLOCKS_PER_SUBCHUNK * 6 * ATTRIBS_PER_FACE :
    //     mm_mesh_size + 256;

    constexpr unsigned int lim = BLOCKS_PER_SUBCHUNK * 6 * ATTRIBS_PER_FACE / 16;
    vertex_attrib_t* data = new vertex_attrib_t[lim];
    unsigned int size = getVertexData(this_chunk, data);
    assert(size <= lim * sizeof(vertex_attrib_t));
    m_mesh.generate(size, data, true, m_X, m_Z);
    delete[] data;
}

// update this function!
// 1. Store a 16x16 array holding the highest non-air block in the chunk. Then, only
//    check blocks below that block. Skip solid blocks because most should be solid.
// 2. call blocks->get_all() at the start of the function and split up the for loops
//    into inner blocks (prevents bounds checking) and outer blocks (we already know
//    where the out of bounds blocks will be)

// TODO: find edges separately to prevent bounds checking
// TODO: restrict y based on highest_solid_block
// TODO: make an index value here (before y for loop) to avoid calling subchunk_index every time
unsigned int Chunk::Subchunk::getVertexData(const Chunk* this_chunk, vertex_attrib_t* data) const {
    vertex_attrib_t* start = data; // record the current byte address
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            for (int y = m_Y * SUBCHUNK_HEIGHT; y < (m_Y + 1) * SUBCHUNK_HEIGHT; ++y) {
                Block::BlockType currentBlock = this_chunk->get(x, y, z);
                assert(currentBlock != Block::BlockType::NO_BLOCK);
                if (currentBlock == Block::BlockType::AIR)
                    continue;
                bool dirHasBlock[(int) NUM_DIRECTIONS] = {
                    !Block::isTransparent(this_chunk->get(x + 1, y, z)),
                    !Block::isTransparent(this_chunk->get(x - 1, y, z)),
                    !Block::isTransparent(this_chunk->get(x, y, z + 1)),
                    !Block::isTransparent(this_chunk->get(x, y, z - 1)),
                    !Block::isTransparent(this_chunk->get(x, y + 1, z)),
                    !Block::isTransparent(this_chunk->get(x, y - 1, z)),
                };
                data += Block::getBlockData(currentBlock, x, y, z, data, dirHasBlock);
            }
        }
    }
    // return the number of bytes that were initialized
    return (unsigned int) ((data - start) * sizeof(vertex_attrib_t));
}
