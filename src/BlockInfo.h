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
        GRASS_PLANT,
        OAK_LOG,
        OAK_LOG_PX,
        OAK_LOG_PZ,
        OAK_LEAVES,
        OUTLINE,
        NO_BLOCK,
        NUM_BLOCK_TYPES,
        // LOG    (log ends are in the +y and -y directions)
        // LOG_PX (log ends are in the +x and -x directions)
        // LOG_PZ (log ends are in the +z and -z directions)
        // Leaf
        // plants
        // slabs
        // ...
    };

    void setBlockData();
    int getBlockData(BlockType type, int x, int y, int z,
                     VertexAttribType* data, bool dirHasBlock[NUM_DIRECTIONS]);

    sglm::vec3 getVertexPosition(const Vertex& vertex);
    sglm::vec3 getBlockPosition(const Vertex& vertex);
    bool isTransparent(BlockType type);

}

#endif
