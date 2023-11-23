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
    int getBlockData(BlockType type, int x, int y, int z,
                     VertexAttribType* data, bool dirHasBlock[NUM_DIRECTIONS]);

    sglm::vec3 getPosition(const Vertex& vertex);
    bool isTransparent(BlockType type);

}

#endif
