#ifndef BLOCK_TYPE_H_INCLUDED
#define BLOCK_TYPE_H_INCLUDED

#include "Constants.h"
#include <sglm/sglm.h>

namespace Block {

    enum class BlockType : unsigned char {
        AIR,
        GRASS,
        DIRT,
        STONE,
        OUTLINE,
        NO_BLOCK,
    };

    void setBlockData();
    void getFaceData(BlockType type, int x, int y, int z, unsigned int* data, Direction face);
    void getBlockData(BlockType type, int x, int y, int z, unsigned int* data);

    sglm::vec3 getPosition(unsigned int vertex);
    bool isTransparent(BlockType type);

}

#endif
