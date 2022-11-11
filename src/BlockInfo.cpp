#include "BlockInfo.h"

#include <iostream>

namespace Block {

    const unsigned int* getData(BlockType type, BlockFace face) {
        int offset = UINTS_PER_FACE * static_cast<int>(face);
        switch (type) {
            case BlockType::GRASS: return GRASS_BLOCK_DATA + offset;
            case BlockType::DIRT:  return DIRT_BLOCK_DATA + offset;
            case BlockType::STONE: return STONE_BLOCK_DATA + offset;
        }
        std::cerr << "Invalid type / No data for this block: " << static_cast<int>(face) << '\n';
        return nullptr;
    }

    bool isTransparent(BlockType type) {
        return type == BlockType::AIR;
    }

}
