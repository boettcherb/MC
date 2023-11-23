#include "BlockInfo.h"
#include "Constants.h"
#include <sglm/sglm.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

// 
// NEW REPRESENTATION:
// Maybe each vertex has a block position within a subchunk (4 bits for each
// coordinate) but also a pixel position within a block (6 bits for each
// coordinate, to allow rendering up to 1 block outside the acutal block for
// things like crops and fences)
// 
// light      (4 bits)
// x position (4 bits)
// y position (4 bits)
// z position (4 bits)
// pixel x    (6 bits)
// pixel y    (6 bits)
// pixel z    (6 bits)
// texture x  (5 bits)
// texture y  (5 bits)
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
        GRASS_TOP, GRASS_SIDES, DIRT, STONE, OUTLINE, NUM_TEXTURES
    };


    enum class FaceType {
        PLUS_X_NORMAL, MINUS_X_NORMAL, PLUS_Z_NORMAL, MINUS_Z_NORMAL, PLUS_Y_NORMAL, MINUS_Y_NORMAL,
        NUM_FACE_TYPES
        // future: PLUS_X_FENCE, PLUS_X_SLAB, PLUS_X_STAIR
    };

    // for each block type, store a direction for each face. This direction is
    // the direction that will determine whether we render this face (if there
    // is a solid block in that direction, don't render the face)
    static std::vector<Direction> DIR[] = {
        {}, // air
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // grass
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // dirt
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // stone
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // outline
    };

    static constexpr int numBlockTypes = (int) BlockType::NO_BLOCK;
    static std::vector<VertexAttribType> blockData[numBlockTypes];

    static void setData(BlockType block) {

        // Store the locations of each texture as a point. This point (x and y
        // range from 0 to 16) corresponds to the texture's location on the
        // texture sheet.
        std::pair<int, int> textures[] = {
            { 2, 6 },
            { 0, 6 },
            { 1, 6 },
            { 3, 6 },
            { 1, 0 },
        };

        std::vector<std::pair<Tex, FaceType>> blocks[] = {
            {},
            {{ Tex::GRASS_SIDES, FaceType::PLUS_X_NORMAL },
             { Tex::GRASS_SIDES, FaceType::MINUS_X_NORMAL },
             { Tex::GRASS_SIDES, FaceType::PLUS_Z_NORMAL },
             { Tex::GRASS_SIDES, FaceType::MINUS_Z_NORMAL },
             { Tex::GRASS_TOP,   FaceType::PLUS_Y_NORMAL },
             { Tex::DIRT,        FaceType::MINUS_Y_NORMAL }},

            {{Tex::DIRT, FaceType::PLUS_X_NORMAL },
             {Tex::DIRT, FaceType::MINUS_X_NORMAL },
             {Tex::DIRT, FaceType::PLUS_Z_NORMAL },
             {Tex::DIRT, FaceType::MINUS_Z_NORMAL },
             {Tex::DIRT, FaceType::PLUS_Y_NORMAL },
             {Tex::DIRT, FaceType::MINUS_Y_NORMAL }},

            {{Tex::STONE, FaceType::PLUS_X_NORMAL },
             {Tex::STONE, FaceType::MINUS_X_NORMAL },
             {Tex::STONE, FaceType::PLUS_Z_NORMAL },
             {Tex::STONE, FaceType::MINUS_Z_NORMAL },
             {Tex::STONE, FaceType::PLUS_Y_NORMAL },
             {Tex::STONE, FaceType::MINUS_Y_NORMAL }},

            {{Tex::OUTLINE, FaceType::PLUS_X_NORMAL },
             {Tex::OUTLINE, FaceType::MINUS_X_NORMAL },
             {Tex::OUTLINE, FaceType::PLUS_Z_NORMAL },
             {Tex::OUTLINE, FaceType::MINUS_Z_NORMAL },
             {Tex::OUTLINE, FaceType::PLUS_Y_NORMAL },
             {Tex::OUTLINE, FaceType::MINUS_Y_NORMAL }},
        };

        constexpr int LIGHT_BITS = 2;
        constexpr int POS_X_BITS = 5;
        constexpr int POS_Y_BITS = 8;
        constexpr int POS_Z_BITS = 5;
        constexpr int TEX_X_BITS = 5;
        constexpr int TEX_Y_BITS = 5;

        // Stores the offsets for the x, y, and z positions of each vertex and the
        // offsets for the x and y texture coordinates of each vertex.
        VertexAttribType offs[][VERTICES_PER_FACE][6] = {
            // +x (normal)
            {{2, 1, 0, 1, 0, 0}, {2, 1, 0, 0, 1, 0}, {2, 1, 1, 0, 1, 1},
             {2, 1, 1, 0, 1, 1}, {2, 1, 1, 1, 0, 1}, {2, 1, 0, 1, 0, 0}},
            // -x (normal)
            {{2, 0, 0, 0, 0, 0}, {2, 0, 0, 1, 1, 0}, {2, 0, 1, 1, 1, 1},
             {2, 0, 1, 1, 1, 1}, {2, 0, 1, 0, 0, 1}, {2, 0, 0, 0, 0, 0}},
            // +z (normal)
            {{1, 0, 0, 1, 0, 0}, {1, 1, 0, 1, 1, 0}, {1, 1, 1, 1, 1, 1},
             {1, 1, 1, 1, 1, 1}, {1, 0, 1, 1, 0, 1}, {1, 0, 0, 1, 0, 0}},
            // -z (normal)
            {{1, 1, 0, 0, 0, 0}, {1, 0, 0, 0, 1, 0}, {1, 0, 1, 0, 1, 1},
             {1, 0, 1, 0, 1, 1}, {1, 1, 1, 0, 0, 1}, {1, 1, 0, 0, 0, 0}},
            // +y (normal)
            {{3, 0, 1, 1, 0, 0}, {3, 1, 1, 1, 1, 0}, {3, 1, 1, 0, 1, 1},
             {3, 1, 1, 0, 1, 1}, {3, 0, 1, 0, 0, 1}, {3, 0, 1, 1, 0, 0}},
            // -y (normal)
            {{0, 0, 0, 0, 0, 0}, {0, 1, 0, 0, 1, 0}, {0, 1, 0, 1, 1, 1},
             {0, 1, 0, 1, 1, 1}, {0, 0, 0, 1, 0, 1}, {0, 0, 0, 0, 0, 0}}
            // future: data for plants, slabs, stairs, fences, torches, etc.
        };

        std::vector<VertexAttribType> data;
        for (int f = 0; f < (int) blocks[(int) block].size(); ++f) {
            auto& [tex, face] = blocks[(int) block][f];
            auto& [texX, texY] = textures[(int) tex];
            for (int v = 0; v < VERTICES_PER_FACE; ++v) {
                VertexAttribType val = 0;
                val = (val << LIGHT_BITS) + offs[(int) face][v][0];
                val = (val << POS_X_BITS) + offs[(int) face][v][1];
                val = (val << POS_Y_BITS) + offs[(int) face][v][2];
                val = (val << POS_Z_BITS) + offs[(int) face][v][3];
                val = (val << TEX_X_BITS) + offs[(int) face][v][4] + texX;
                val = (val << TEX_Y_BITS) + offs[(int) face][v][5] + texY;
                data.push_back(val);
                for (int i = 0; i < ATTRIBS_PER_VERTEX - 1; ++i)
                    data.push_back(0);
            }
        }
        blockData[(int) block] = data;
    }

    void setBlockData() {
        for (int blockType = 1; blockType <= (int) BlockType::OUTLINE; ++blockType) {
            setData(static_cast<BlockType>(blockType));
        }
    }
    
    // Return the number of VertexAttribTypes set in data
    int getBlockData(BlockType type, int x, int y, int z,
                     VertexAttribType* data, bool dirHasBlock[NUM_DIRECTIONS]) {
        int size = 0;
        // std::vector<std::tuple<Tex, FaceType, Direction>>& faces = blocks[(int) type];
        std::vector<Direction>& d = DIR[(int) type];
        int numFaces = (int) d.size();
        std::vector<VertexAttribType>& curBlockData = blockData[(int) type];

        for (int face = 0; face < (int) numFaces; ++face) {
            // if there is a block in the face's direction, don't retrieve its
            // data (because it is hidden by the block)
            Direction dir = d[face];
            assert(dir >= 0);
            if (dir < NUM_DIRECTIONS && dirHasBlock[dir])
                continue;
            // No block, so retrieve the face's data
            int offset = face * ATTRIBS_PER_FACE;
            for (int v = 0; v < ATTRIBS_PER_FACE; v += ATTRIBS_PER_VERTEX) {
                data[size + v] = curBlockData[offset + v];
                data[size + v] += (x << 23) + (y << 15) + (z << 10);
                data[size + v + 1] = curBlockData[offset + v + 1];
                data[size + v + 2] = curBlockData[offset + v + 2];
                
            }
            size += ATTRIBS_PER_FACE;
        }
        return size;
    }

    sglm::vec3 getPosition(const Vertex& vertex) {
        VertexAttribType first = vertex.v1;
        float x = (float) ((first >> 23) & 0x1F);
        float y = (float) ((first >> 15) & 0xFF);
        float z = (float) ((first >> 10) & 0x1F);
        return { x, y, z };
    };

    bool isTransparent(BlockType type) {
        return type == BlockType::AIR;
    }

}
