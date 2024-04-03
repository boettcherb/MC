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

Chunk::BlockList::BlockList(const Block::BlockType* blocks, int size) {
    assert(blocks != nullptr);
    m_index.fill(NO_BLOCK);
    m_size = size;
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
    assert(m_data != nullptr);
    auto [data, bit_index] = get_data(index(x, y, z));
    data >>= (64 - m_bits_per_block - bit_index);
    return m_palette[data & m_bitmask];
}

void Chunk::BlockList::put(int x, int y, int z, Block::BlockType block) {
    assert(m_data != nullptr);
    add_block(block, true);
    auto [data, bit_index] = get_data(index(x, y, z));
    int i = index(x, y, z) * m_bits_per_block / 32;
    int shift = 64 - m_bits_per_block - bit_index;
    data &= ~(m_bitmask << shift); // 0-out the current block
    data |= static_cast<uint64>(m_index[(int) block]) << shift;
    assert(m_index[(int) block] <= std::pow(2, m_bits_per_block));
    m_data[i] = static_cast<unsigned int>(data >> 32);
    m_data[i + 1] = static_cast<unsigned int>(data & 0x00000000FFFFFFFF);
}

// Convert the block data back into a Block::BlockType array. The caller is
// responsible for freeing the returned array.
Block::BlockType* Chunk::BlockList::get_all() const {
    assert(m_data != nullptr);
    Block::BlockType* blockList = new Block::BlockType[BLOCKS_PER_CHUNK];
    uint64 data = (static_cast<uint64>(m_data[0]) << 32) | m_data[1];
    int cur_bit = 64;
    int data_index = 2;
    for (int i = 0; i < BLOCKS_PER_CHUNK; ++i) {
        int shift = cur_bit - m_bits_per_block;
        blockList[i] = m_palette[(data >> shift) & m_bitmask];
        cur_bit -= m_bits_per_block;
        if (cur_bit <= 32) {
            cur_bit += 32;
            data = (data << 32) | m_data[data_index++];
        }
    }
    return blockList;
}

std::pair<uint64, int> Chunk::BlockList::get_data(int block_index) const {
    int index = block_index * m_bits_per_block / 32;
    uint64 data = m_data[index];
    data = (data << 32) | static_cast<uint64>(m_data[index + 1]);
    int bit_index = block_index * m_bits_per_block - index * 32;
    return { data, bit_index };
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
    m_data = new unsigned int[m_bits_per_block * m_size / 32 + 2];

    // fill in m_data
    int bits_set = 0;
    int data_index = 0;
    uint64 data = 0;
    for (int i = 0; i < BLOCKS_PER_CHUNK; ++i) {
        data = (data << m_bits_per_block) | m_index[(int) blocks[i]];
        bits_set += m_bits_per_block;
        if (bits_set >= 32) {
            bits_set -= 32;
            m_data[data_index++] = static_cast<unsigned int>((data >> bits_set) & 0x00000000FFFFFFFF);
        }
    }
    assert(bits_set < 32);
    m_data[data_index++] = static_cast<unsigned int>((data << (32 - bits_set)) & 0x00000000FFFFFFFF);
}
