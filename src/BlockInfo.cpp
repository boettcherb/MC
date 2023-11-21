#include "BlockInfo.h"
#include "Constants.h"
#include <sglm/sglm.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <array>

// 
// NEW REPRESENTATION:
// Maybe each vertex has a block position within a subchunk (4 bits for each
// coordinate) but also a pixel position within a block (6 bits for each
// coordinate, to allow rendering up to 1 block outside the acutal block for
// things like crops and fences)
// 
// x position (4 bits)
// y position (4 bits)
// z position (4 bits)
// pixel x    (6 bits)
// pixel y    (6 bits)
// pixel z    (6 bits)
// texture x  (5 bits)
// texture y  (5 bits)
// light      (4 bits)
// 

//
// A vertex is represented by 1 32-bit unsigned integer:
// light val  (2 bits): 00110000000000000000000000000000
// x position (5 bits): 00001111100000000000000000000000
// y position (8 bits): 00000000011111111000000000000000
// z position (5 bits): 00000000000000000111110000000000
// x texcoord (5 bits): 00000000000000000000001111100000
// y texcoord (5 bits): 00000000000000000000000000011111
//
// The light value is an index into an array of values from 0 to 1 that
// represent the intensity of light hitting the block face. 1 is full
// brightness and 0 is full darkness (array is defined in vertex shader).
// 
// The x and z positions are values from 0 to 16 and the y position is a value
// from 0 to 128. These represent the position of the vertex within a chunk.
//
// The texture coordinates also range from 0 to 16. The vertex shader divides
// these values by 16 and the results (floats from 0 to 1) determine where in
// the texture to sample from. (0, 0) is bottom left and (1, 1) is top right.
//

namespace Block {

    enum class Tex {
        GRASS_TOP, GRASS_SIDES, DIRT, STONE, OUTLINE
    };

    // Store the locations of each texture in the texture sheet as a point
    // (x and y range from 0 to 16)
    std::map<Tex, std::pair<int, int>> textures = {
        {Tex::GRASS_SIDES, {0, 6}},
        {Tex::DIRT,        {1, 6}},
        {Tex::GRASS_TOP,   {2, 6}},
        {Tex::STONE,       {3, 6}},
        {Tex::OUTLINE,     {1, 0}},
    };

    //
    // For each block type, store the textures on each face of the block. Textures
    // are listed in this order: (+x, -x, +z, -z, +y, -y), and we are assuming that
    // the block is in a default rotation facing the +x direction.
    // 
    // Most blocks are the same on all 6 sides. For example, a dirt block has the
    // dirt texture (Tex::DIRT) on every side. However, some blocks, such as the
    // chest, are different on most sides. This is where the ordering above is
    // important. We are assuming that the 'forward' direction of the block is +x
    // and the 'up' direction is +y. 
    //
    std::map<BlockType, std::array<Tex, 6>> blocks = {
        {BlockType::GRASS,   {Tex::GRASS_SIDES, Tex::GRASS_SIDES, Tex::GRASS_SIDES, Tex::GRASS_SIDES, Tex::GRASS_TOP, Tex::DIRT}},
        {BlockType::DIRT,    {Tex::DIRT,        Tex::DIRT,        Tex::DIRT,        Tex::DIRT,        Tex::DIRT,      Tex::DIRT}},
        {BlockType::STONE,   {Tex::STONE,       Tex::STONE,       Tex::STONE,       Tex::STONE,       Tex::STONE,     Tex::STONE}},
        {BlockType::OUTLINE, {Tex::OUTLINE,     Tex::OUTLINE,     Tex::OUTLINE,     Tex::OUTLINE,     Tex::OUTLINE,   Tex::OUTLINE}},
    };

    inline constexpr int LIGHT_BITS = 2;
    inline constexpr int POS_X_BITS = 5;
    inline constexpr int POS_Y_BITS = 8;
    inline constexpr int POS_Z_BITS = 5;
    inline constexpr int TEX_X_BITS = 5;
    inline constexpr int TEX_Y_BITS = 5;

    // Stores the offsets for the x, y, and z positions of each vertex and the
    // offsets for the x and y texture coordinates of each vertex.
    int offs[6][6][5] = {
        {{1, 0, 1, 0, 0}, {1, 0, 0, 1, 0}, {1, 1, 0, 1, 1},    // +x
         {1, 1, 0, 1, 1}, {1, 1, 1, 0, 1}, {1, 0, 1, 0, 0}},
        {{0, 0, 0, 0, 0}, {0, 0, 1, 1, 0}, {0, 1, 1, 1, 1},    // -x
         {0, 1, 1, 1, 1}, {0, 1, 0, 0, 1}, {0, 0, 0, 0, 0}},
        {{0, 0, 1, 0, 0}, {1, 0, 1, 1, 0}, {1, 1, 1, 1, 1},    // +z
         {1, 1, 1, 1, 1}, {0, 1, 1, 0, 1}, {0, 0, 1, 0, 0}},
        {{1, 0, 0, 0, 0}, {0, 0, 0, 1, 0}, {0, 1, 0, 1, 1},    // -z
         {0, 1, 0, 1, 1}, {1, 1, 0, 0, 1}, {1, 0, 0, 0, 0}},
        {{0, 1, 1, 0, 0}, {1, 1, 1, 1, 0}, {1, 1, 0, 1, 1},    // +y
         {1, 1, 0, 1, 1}, {0, 1, 0, 0, 1}, {0, 1, 1, 0, 0}},
        {{0, 0, 0, 0, 0}, {1, 0, 0, 1, 0}, {1, 0, 1, 1, 1},    // -y
         {1, 0, 1, 1, 1}, {0, 0, 1, 0, 1}, {0, 0, 0, 0, 0}}
    };

    std::vector<unsigned int> getData(BlockType block, Direction up = PLUS_Y, Direction forward = PLUS_X) {
        // assuming default orientation
        (void) up, forward;

        // for now, light value is 3 for top, 2 +x/-x, 1 for +z/-z, and 0 for bottom
        int light[VERTICES_PER_FACE] = { 2, 2, 1, 1, 3, 0 };
        int faces = FACES_PER_BLOCK; // assume 6 for now; might change when adding plants

        std::vector<unsigned int> data;
        for (int f = 0; f < faces; ++f) {
            auto& [texX, texY] = textures[blocks[block][f]];
            for (int v = 0; v < VERTICES_PER_FACE; ++v) {
                unsigned int val = light[f];
                val = (val << POS_X_BITS) + offs[f][v][0];
                val = (val << POS_Y_BITS) + offs[f][v][1];
                val = (val << POS_Z_BITS) + offs[f][v][2];
                val = (val << TEX_X_BITS) + texX + offs[f][v][3];
                val = (val << TEX_Y_BITS) + texY + offs[f][v][4];
                data.push_back(val);
            }
        }
        return data;
    }

    void getFaceData(BlockType type, int x, int y, int z, unsigned int* data, Direction face) {
        assert(face >= 0 && face < 6);
        assert(type != BlockType::AIR && type != BlockType::NO_BLOCK);
        int offset = UINTS_PER_FACE * (int) face;
        std::vector<unsigned int> blockData = getData(type);
        for (int i = 0; i < FACES_PER_BLOCK; ++i) {
            data[i] = blockData[offset + i];
        }
        for (int i = 0; i < UINTS_PER_FACE; ++i) {
            data[i] += (x << 23) + (y << 15) + (z << 10);
        }
    }

    void getBlockData(BlockType type, int x, int y, int z, unsigned int* data) {
        getFaceData(type, x, y, z, data, PLUS_X); data += UINTS_PER_FACE;
        getFaceData(type, x, y, z, data, MINUS_X); data += UINTS_PER_FACE;
        getFaceData(type, x, y, z, data, PLUS_Z); data += UINTS_PER_FACE;
        getFaceData(type, x, y, z, data, MINUS_Z); data += UINTS_PER_FACE;
        getFaceData(type, x, y, z, data, PLUS_Y); data += UINTS_PER_FACE;
        getFaceData(type, x, y, z, data, MINUS_Y);
    }

    sglm::vec3 getPosition(unsigned int vertex) {
        unsigned int x = (vertex >> 23) & 0x1F;
        unsigned int y = (vertex >> 15) & 0xFF;
        unsigned int z = (vertex >> 10) & 0x1F;
        return { (float) x, (float) y, (float) z };
    };

    bool isTransparent(BlockType type) {
        return type == BlockType::AIR;
    }

}
