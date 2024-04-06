#include "Chunk.h"
#include "BlockInfo.h"

#include <cmath>
#include <cassert>

// Methods for the class Chunk::BlockList. This class is used to reduce the
// memory usage of storing each block in a chunk. This class finds the block
// types that are used in a subchunk. Example: { Stone, Dirt, Grass, Oak Log, Oak
// Leaf }. Then, it stores an index into this 'palette' of blocks (which might be
// only 3 or 4 bits depending on how many block types are in the subchunk) instead
// of storing the actual block ids (which are currently 8 bits but might increase
// to 16 bits if there are more than 256 block types.

static constexpr int NO_BLOCK = (int) Block::BlockType::NO_BLOCK;
typedef unsigned long long uint64;

Chunk::BlockList::BlockList(const Block::BlockType* blocks, int size) : m_size{ size } {
    assert(blocks != nullptr);
    m_index.fill(NO_BLOCK);
    for (int i = 0; i < size; ++i) {
        add_block(blocks[i], false);
    }
    m_data = nullptr;
    m_bits_per_block = 0;
    m_bitmask = 0;
    build(blocks);
}

Chunk::BlockList::~BlockList() {
    delete[] m_data;
}

Block::BlockType Chunk::BlockList::get(int x, int y, int z) const {
    // int block_index = Chunk::subchunk_index(x, y, z);
    int block_index = Chunk::chunk_index(x, y, z);
    int data_index = block_index / m_blocks_per_ll;
    int i = block_index % m_blocks_per_ll;
    return m_palette[(m_data[data_index] >> (m_bits_per_block * i)) & m_bitmask];
}

void Chunk::BlockList::put(int x, int y, int z, Block::BlockType block) {
    add_block(block, true);
    // int block_index = Chunk::subchunk_index(x, y, z);
    int block_index = Chunk::chunk_index(x, y, z);
    int data_index = block_index / m_blocks_per_ll;
    int i = block_index % m_blocks_per_ll;
    int shift = m_bits_per_block * i;
    m_data[data_index] &= ~(m_bitmask << shift);
    m_data[data_index] |= ((uint64) m_index[(int) block]) << shift;
}

// Convert the block data back into a Block::BlockType array. The caller is
// responsible for freeing the returned array.
Block::BlockType* Chunk::BlockList::get_all() const {
    int block_index = 0;
    Block::BlockType* blockList = new Block::BlockType[m_size];
    for (int i = 0; i < m_data_size; ++i) {
        uint64 cur = m_data[i];
        for (int j = 0; j < m_blocks_per_ll; ++j) {
            if (block_index >= m_size) {
                assert(i == m_data_size - 1);
                break;
            }
            blockList[block_index++] = m_palette[cur & m_bitmask];
            cur >>= m_bits_per_block;
        }
    }
    return blockList;
}

void Chunk::BlockList::add_block(Block::BlockType block, bool rebuild) {
    int b = (int) block;
    if (m_index[b] == NO_BLOCK) {
        m_index[b] = (int) m_palette.size();
        m_palette.push_back(block);
        if (rebuild) {
            build(nullptr);
        }
    }
}

void Chunk::BlockList::build(const Block::BlockType* blocks) {
    assert(m_palette.size() > 0);
    int num_bits = static_cast<int>(std::ceil(std::log2(m_palette.size())));
    if (num_bits == 0)
        num_bits = 1;
    assert(num_bits > 0 && num_bits <= 16);
    if (num_bits == m_bits_per_block) {
        return;
    }
    assert(m_bits_per_block < num_bits);
    // rebuild using the existing data in m_data
    if (blocks == nullptr) {
        Block::BlockType* blockList = get_all();
        fill_data(blockList, num_bits);
        delete[] blockList;
    } else {
        fill_data(blocks, num_bits);
    }
}

// create m_data and fill it with the given blocks
void Chunk::BlockList::fill_data(const Block::BlockType* blocks, int num_bits) {
    m_bits_per_block = num_bits;
    assert(std::pow(2, m_bits_per_block) >= m_palette.size());
    m_bitmask = static_cast<uint64>(std::pow(2, m_bits_per_block) - 1);
    delete[] m_data;

    // fit as many blocks into a 64-bit integer as we can,
    // without overflowing into the next one.
    m_blocks_per_ll = 64 / num_bits;
    m_data_size = m_size / m_blocks_per_ll + (m_size % m_blocks_per_ll != 0);
    m_data = new uint64[m_data_size];

    // fill in m_data
    int block_index = 0;
    for (int i = 0; i < m_data_size; ++i) {
        m_data[i] = 0;
        int shift = 0;
        for (int j = 0; j < m_blocks_per_ll; ++j) {
            if (block_index >= m_size) {
                assert(i == m_data_size - 1);
                break;
            }
            uint64 curBlock = m_index[(int) blocks[block_index++]];
            m_data[i] |= curBlock << shift;
            shift += num_bits;
        }
    }
}
