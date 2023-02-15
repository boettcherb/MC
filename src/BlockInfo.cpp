#include "BlockInfo.h"
#include "Constants.h"
#include <cassert>

namespace Block {

    const unsigned int* getData(BlockType type, BlockFace face) {
        int offset = UINTS_PER_FACE * (int) face;
        switch (type) {
            case BlockType::GRASS: return GRASS_BLOCK_DATA + offset;
            case BlockType::DIRT:  return DIRT_BLOCK_DATA + offset;
            case BlockType::STONE: return STONE_BLOCK_DATA + offset;
        }
        assert(0 && "Invalid type / No data for this block");
        return nullptr;
    }

    bool isTransparent(BlockType type) {
        return type == BlockType::AIR;
    }

}
