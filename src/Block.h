#ifndef BLOCK_TYPE_H_INCLUDED
#define BLOCK_TYPE_H_INCLUDED

#include "Constants.h"
#include <sglm/sglm.h>
#include <array>

namespace Block {

    enum class BlockType : unsigned char {
        AIR,
        GRASS,
        DIRT,
        STONE,
        SAND,
        SNOW,
        WATER,
        GRASS_PLANT,
        BLUE_FLOWER,
        PINK_FLOWER,
        RED_FLOWER,
        CACTUS,
        DEAD_BUSH,
        OAK_LOG,
        OAK_LOG_PX,
        OAK_LOG_PZ,
        OAK_LEAVES,
        JUNGLE_LOG,
        JUNGLE_LOG_PX,
        JUNGLE_LOG_PZ,
        JUNGLE_LEAVES,
        OUTLINE,
        NUM_BLOCK_TYPES,
        NO_BLOCK,
        // LOG    (log ends are in the +y and -y directions)
        // LOG_PX (log ends are in the +x and -x directions)
        // LOG_PZ (log ends are in the +z and -z directions)
        // Leaf
        // plants
        // slabs
        // ...
    };

    void initBlockData();
    int getBlockData(BlockType type, int x, int y, int z, vertex_attrib_t* data,
                     const std::array<BlockType, NUM_DIRECTIONS>& surrounding);

    sglm::vec3 getVertexPosition(const Vertex& vertex);
    sglm::vec3 getBlockPosition(const Vertex& vertex);
    bool isReal(BlockType type);
    bool isNormal(BlockType type);
    bool isSolid(BlockType type);

}

#endif
