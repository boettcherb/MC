#include "Chunk.h"
#include "BlockInfo.h"
#include "Constants.h"

#include <iostream>
#include <cassert>

Chunk::Subchunk::Subchunk(int y, const Block::BlockType* data) : m_Y{ y },
    m_blocks { BlockList(data, BLOCKS_PER_SUBCHUNK) } {
    m_mesh_size = -1;
}

void Chunk::Subchunk::updateMesh(const Chunk* this_chunk) {
    m_mesh.erase();
    unsigned int lim = m_mesh_size == -1 ?
        40000 :
        m_mesh_size + 1024;
    vertex_attrib_t* data = nullptr;
    while (true) {
        try {
            data = new vertex_attrib_t[lim];
            unsigned int size = getVertexData(this_chunk, lim * sizeof(vertex_attrib_t), data);
            assert(size <= lim * sizeof(vertex_attrib_t));
            m_mesh.generate(size, data, true, this_chunk->m_posX, this_chunk->m_posZ);
            m_mesh_size = size / sizeof(vertex_attrib_t);
            delete[] data;
            break;
        } catch (const char* error_str) {
            std::cout << "-----------" << std::endl;
            std::cout << "Error: " << error_str << std::endl;
            std::cout << "lim: " << lim << ", m_mesh_size: " << m_mesh_size << std::endl;
            std::cout << "-----------" << std::endl;
            lim *= 2;
            delete[] data;
        }
    }
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
unsigned int Chunk::Subchunk::getVertexData(const Chunk* this_chunk, int byte_lim,
                                            vertex_attrib_t* data) const {
    vertex_attrib_t* start = data; // record the current byte address
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            for (int y = m_Y * SUBCHUNK_HEIGHT; y < (m_Y + 1) * SUBCHUNK_HEIGHT; ++y) {
                Block::BlockType currentBlock = this_chunk->get(x, y, z);
                assert(currentBlock != Block::BlockType::NO_BLOCK);
                if (currentBlock == Block::BlockType::AIR)
                    continue;
                std::array<Block::BlockType, NUM_DIRECTIONS> surrounding = {
                    this_chunk->get(x + 1, y, z),
                    this_chunk->get(x - 1, y, z),
                    this_chunk->get(x, y, z + 1),
                    this_chunk->get(x, y, z - 1),
                    this_chunk->get(x, y + 1, z),
                    this_chunk->get(x, y - 1, z),
                };
                data += Block::getBlockData(currentBlock, x, y, z, data, surrounding);

                // if we are almost about to go over the byte limit, don't risk it 
                if ((int) ((data - start) * sizeof(vertex_attrib_t)) > byte_lim - 256) {
                    throw "byte_lim too small";
                }
            }
        }
    }
    // return the number of bytes that were initialized
    return (unsigned int) ((data - start) * sizeof(vertex_attrib_t));
}
