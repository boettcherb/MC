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
        100000 :
        m_mesh_size + 1024;
    vertex_attrib_t* data = nullptr;
    while (true) {
        try {
            data = new vertex_attrib_t[lim];
            unsigned int size = getVertexData(this_chunk, lim * sizeof(vertex_attrib_t), data);
            assert(size <= lim * sizeof(vertex_attrib_t));
            m_mesh.generate(size, data, true, this_chunk->m_posX, m_Y, this_chunk->m_posZ);
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

static inline bool inbounds(int x, int y, int z) {
    return x > 0 && y > 0 && z > 0 && x < CHUNK_WIDTH - 1 &&
        y < SUBCHUNK_HEIGHT - 1 && z < CHUNK_WIDTH - 1;
}

unsigned int Chunk::Subchunk::getVertexData(const Chunk* this_chunk, int byte_lim,
                                            vertex_attrib_t* data) const {
    vertex_attrib_t* start = data; // record the current byte address
    Block::BlockType* blocks = m_blocks.get_all();
    int y_offs = m_Y * SUBCHUNK_HEIGHT;

    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            // int y_max = std::min(y_offs + SUBCHUNK_HEIGHT - 1, (int) this_chunk->m_highest_block[x][z]);
            // y_max -= y_offs;
            int index = Chunk::subchunk_index(x, 0, z);
            // for (int y = 0; y <= y_max; ++y) {
            for (int y = 0; y < SUBCHUNK_HEIGHT; ++y) {
                Block::BlockType block = blocks[index];
                assert(Block::isReal(block));
                if (block == Block::BlockType::AIR) {
                    ++index;
                    continue;
                }
                if (inbounds(x, y, z)) {
                    std::array<Block::BlockType, NUM_DIRECTIONS> surrounding = {
                        blocks[index + CHUNK_WIDTH * SUBCHUNK_HEIGHT],
                        blocks[index - CHUNK_WIDTH * SUBCHUNK_HEIGHT],
                        blocks[index + SUBCHUNK_HEIGHT],
                        blocks[index - SUBCHUNK_HEIGHT],
                        blocks[index + 1],
                        blocks[index - 1],
                    };
                    data += Block::getBlockData(block, x, y, z, data, surrounding);
                }
                else {
                    std::array<Block::BlockType, NUM_DIRECTIONS> surrounding = {
                        this_chunk->get(x + 1, y + y_offs,     z    ),
                        this_chunk->get(x - 1, y + y_offs,     z    ),
                        this_chunk->get(x,     y + y_offs,     z + 1),
                        this_chunk->get(x,     y + y_offs,     z - 1),
                        this_chunk->get(x,     y + y_offs + 1, z    ),
                        this_chunk->get(x,     y + y_offs - 1, z    ),
                    };
                    data += Block::getBlockData(block, x, y, z, data, surrounding);
                }
                // if we are almost about to go over the byte limit, don't risk it 
                if ((int) ((data - start) * sizeof(vertex_attrib_t)) > byte_lim - 512) {
                    throw "byte_lim too small";
                }
                ++index;
            }
        }
    }
    delete[] blocks;
    // return the number of bytes that were initialized
    return (unsigned int) ((data - start) * sizeof(vertex_attrib_t));
}
