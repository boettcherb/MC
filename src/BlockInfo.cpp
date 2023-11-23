#include "BlockInfo.h"
#include "Constants.h"
#include <sglm/sglm.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <tuple>

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
        GRASS_TOP, GRASS_SIDES, DIRT, STONE, OUTLINE
    };

    // Store the locations of each texture as a point. This point (x and y
    // range from 0 to 16) corresponds to the texture's location on the
    // texture sheet.

    static std::map<Tex, std::pair<int, int>> textures = {
        {Tex::GRASS_SIDES, {0, 6}},
        {Tex::DIRT,        {1, 6}},
        {Tex::GRASS_TOP,   {2, 6}},
        {Tex::STONE,       {3, 6}},
        {Tex::OUTLINE,     {1, 0}},
    };

    enum class FaceType {
        PLUS_X_NORMAL, MINUS_X_NORMAL, PLUS_Z_NORMAL, MINUS_Z_NORMAL, PLUS_Y_NORMAL, MINUS_Y_NORMAL,
        NUM_FACE_TYPES
        // future: PLUS_X_FENCE, PLUS_X_SLAB, PLUS_X_STAIR
    };

    // For each block type, store a vector listing its faces (most have 6 faces,
    // but some blocks, such as fences, stairs, and plants, can have different
    // amounts). For each face, store the face's texture and a direction. If there
    // is a full, non-transparent block next to the current block in the given
    // direction, then DONT render the face.

    static std::map<BlockType, std::vector<std::tuple<Tex, FaceType, Direction>>> blocks = {
        {BlockType::GRASS,   {{ Tex::GRASS_SIDES, FaceType::PLUS_X_NORMAL,  PLUS_X },
                              { Tex::GRASS_SIDES, FaceType::MINUS_X_NORMAL, MINUS_X },
                              { Tex::GRASS_SIDES, FaceType::PLUS_Z_NORMAL,  PLUS_Z },
                              { Tex::GRASS_SIDES, FaceType::MINUS_Z_NORMAL, MINUS_Z },
                              { Tex::GRASS_TOP,   FaceType::PLUS_Y_NORMAL,  PLUS_Y },
                              { Tex::DIRT,        FaceType::MINUS_Y_NORMAL, MINUS_Y }}},

        {BlockType::DIRT,    {{Tex::DIRT, FaceType::PLUS_X_NORMAL,  PLUS_X},
                              {Tex::DIRT, FaceType::MINUS_X_NORMAL, MINUS_X},
                              {Tex::DIRT, FaceType::PLUS_Z_NORMAL,  PLUS_Z},
                              {Tex::DIRT, FaceType::MINUS_Z_NORMAL, MINUS_Z},
                              {Tex::DIRT, FaceType::PLUS_Y_NORMAL,  PLUS_Y },
                              {Tex::DIRT, FaceType::MINUS_Y_NORMAL, MINUS_Y }}},
        
        {BlockType::STONE,   {{Tex::STONE, FaceType::PLUS_X_NORMAL,  PLUS_X},
                              {Tex::STONE, FaceType::MINUS_X_NORMAL, MINUS_X},
                              {Tex::STONE, FaceType::PLUS_Z_NORMAL,  PLUS_Z},
                              {Tex::STONE, FaceType::MINUS_Z_NORMAL, MINUS_Z},
                              {Tex::STONE, FaceType::PLUS_Y_NORMAL,  PLUS_Y },
                              {Tex::STONE, FaceType::MINUS_Y_NORMAL, MINUS_Y }}},

        {BlockType::OUTLINE, {{Tex::OUTLINE, FaceType::PLUS_X_NORMAL,  PLUS_X},
                              {Tex::OUTLINE, FaceType::MINUS_X_NORMAL, MINUS_X},
                              {Tex::OUTLINE, FaceType::PLUS_Z_NORMAL,  PLUS_Z},
                              {Tex::OUTLINE, FaceType::MINUS_Z_NORMAL, MINUS_Z},
                              {Tex::OUTLINE, FaceType::PLUS_Y_NORMAL,  PLUS_Y },
                              {Tex::OUTLINE, FaceType::MINUS_Y_NORMAL, MINUS_Y }}},
    };

    static inline constexpr int LIGHT_BITS = 2;
    static inline constexpr int POS_X_BITS = 5;
    static inline constexpr int POS_Y_BITS = 8;
    static inline constexpr int POS_Z_BITS = 5;
    static inline constexpr int TEX_X_BITS = 5;
    static inline constexpr int TEX_Y_BITS = 5;

    // Stores the offsets for the x, y, and z positions of each vertex and the
    // offsets for the x and y texture coordinates of each vertex.
    static VertexAttribType offs[(int) FaceType::NUM_FACE_TYPES][VERTICES_PER_FACE][6] = {
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

    static std::map<BlockType, std::vector<VertexAttribType>> blockData;

    static void setData(BlockType block) {
        std::vector<VertexAttribType> data;
        for (int f = 0; f < (int) blocks[block].size(); ++f) {
            auto& [tex, face, _] = blocks[block][f];
            auto& [texX, texY] = textures[tex];
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
        blockData[block] = data;
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
        for (int face = 0; face < (int) blocks[type].size(); ++face) {
            // if there is a block in the face's direction, don't retrieve its
            // data (because it is hidden by the block)
            Direction dir = std::get<2>(blocks[type][face]);
            assert(dir >= 0);
            if (dir < NUM_DIRECTIONS && dirHasBlock[dir])
                continue;
            // No block, so retrieve the face's data
            int offset = face * ATTRIBS_PER_FACE;
            for (int v = 0; v < ATTRIBS_PER_FACE; v += ATTRIBS_PER_VERTEX) {
                data[size + v] = blockData[type][offset + v];
                data[size + v] += (x << 23) + (y << 15) + (z << 10);
                data[size + v + 1] = blockData[type][offset + v + 1];
                data[size + v + 2] = blockData[type][offset + v + 2];
                
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
