#include "BlockInfo.h"
#include "Constants.h"
#include <sglm/sglm.h>

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>

// A vertex is represented using 3 16-bit integers:
// 
// v1: x pos: 1111000000000000
//     y pos: 0000000011110000
//     z pos: 0000000000001111
// 
// v2: light: 1111000000000000
//     x pix: 0000111111000000
//     y pix: 0000000000111111
// 
// v3: z pix: 1111110000000000
//     x tex: 0000001111100000
//     y tex: 0000000000011111
// 
// The x and z positions are values from 0 to 15 and the y position is a value
// from 0 to 127. These represent the position (within a single chunk) of the
// block that contains the vertex.
// 
// The x, y, and z pixel values represent the position of the vertex within a
// block. These values are stored using 6 bits in order to allow for vertex
// positions outside of a normal block (for example: crops, fences)
// 
// The light value is an index into an array of values from 0 to 1 that
// represent the intensity of light hitting the block face. 1 is full
// brightness and 0 is full darkness (array is defined in vertex shader).
//
// The texture coordinates range from 0 to 16. The vertex shader divides
// these values by 16 and the results (floats from 0 to 1) determine where in
// the texture to sample from. (0, 0) is bottom left and (1, 1) is top right.
//

namespace Block {

    enum class Tex {
        GRASS_TOP, GRASS_SIDES, DIRT, STONE, GRASS_PLANT,
        OAK_LOG, OAK_LOG_END, OAK_LEAVES,
        OUTLINE, NUM_TEXTURES
    };

    enum class FaceType {
        PLUS_X_NORMAL, MINUS_X_NORMAL,
        PLUS_Z_NORMAL, MINUS_Z_NORMAL,
        PLUS_Y_NORMAL, MINUS_Y_NORMAL,
        MXMZ_TO_PXPZ_PLANT, PXPZ_TO_MXMZ_PLANT,
        MXPZ_TO_PXMZ_PLANT, PXMZ_TO_MXPZ_PLANT,
        NUM_FACE_TYPES
        // future: PLUS_X_FENCE, PLUS_X_SLAB, PLUS_X_STAIR
    };

    static constexpr int NUM_BLOCK_TYPES = (int) BlockType::NUM_BLOCK_TYPES;
    static constexpr int NUM_FACE_TYPES = (int) FaceType::NUM_FACE_TYPES;
    static std::vector<Vertex> blockData[NUM_BLOCK_TYPES];

    // for each block type, store a direction for each face. This direction is
    // the direction that will determine whether we render this face. If there
    // is a solid block in that direction, don't render the face. If NO_DIR
    // is specified, always render the face
    static const std::vector<Direction> DIR[NUM_BLOCK_TYPES] = {
        {},                                                    // Air
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // Grass
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // Dirt
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // Stone
        { NO_DIR, NO_DIR, NO_DIR, NO_DIR },                    // Grass Plant
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // Oak Log
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // Oak Log PX
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // Oak Log PZ
        { PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y }, // Oak Leaves
        { NO_DIR, NO_DIR, NO_DIR, NO_DIR, NO_DIR, NO_DIR },    // Block Outline
        // examples for future blocks:
        // 1. OAK_SLAB_BOTTOM
        //      NO_DIR, NO_DIR, NO_DIR, NO_DIR, NO_DIR, MINUS_Y }
        // 2. OAK_STAIR_BPX (bottom with the stair portion in the PLUS_X direction)
        //      Faces: bottom,  back,    top_back, top_front, left_bottom, left_top,   right_bottom, right_top,  front_bottom, front_top
        //        DIR: MINUS_Y, MINUS_X, PLUS_Y,   NO_DIR,    P_Z or M_Z,  P_Z or M_Z, P_Z or M_Z,   P_Z or M_Z, PLUS_X,       NO_DIR

    };

    static void setData(BlockType block) {
        assert(isReal(block) || block == BlockType::OUTLINE);
        // Store the locations of each texture as a point. This point (x and y
        // range from 0 to 16) corresponds to the texture's location on the
        // texture sheet (its bottom left corner).
        // Index into this array with the Tex enum.
        std::pair<int, int> textures[] = {
            { 2, 15 }, // Grass Top
            { 0, 15 }, // Grass Sides
            { 1, 15 }, // Dirt
            { 3, 15 }, // Stone
            { 0, 13 }, // Grass Plant
            { 0, 14 }, // Oak Log
            { 1, 14 }, // Oak Log End
            { 2, 14 }, // Oak Leaves
            { 1, 0 },  // Block Outline
        };
        
        // for each face of each block, store a texture and a face type. The
        // face type is used to index into the offs array (below).
        std::vector<std::pair<Tex, FaceType>> blocks[NUM_BLOCK_TYPES] = {
            // air:
            {},
            // grass:
            {{ Tex::GRASS_SIDES, FaceType::PLUS_X_NORMAL },
             { Tex::GRASS_SIDES, FaceType::MINUS_X_NORMAL },
             { Tex::GRASS_SIDES, FaceType::PLUS_Z_NORMAL },
             { Tex::GRASS_SIDES, FaceType::MINUS_Z_NORMAL },
             { Tex::GRASS_TOP,   FaceType::PLUS_Y_NORMAL },
             { Tex::DIRT,        FaceType::MINUS_Y_NORMAL }},
            // dirt
            {{ Tex::DIRT, FaceType::PLUS_X_NORMAL },
             { Tex::DIRT, FaceType::MINUS_X_NORMAL },
             { Tex::DIRT, FaceType::PLUS_Z_NORMAL },
             { Tex::DIRT, FaceType::MINUS_Z_NORMAL },
             { Tex::DIRT, FaceType::PLUS_Y_NORMAL },
             { Tex::DIRT, FaceType::MINUS_Y_NORMAL }},
            // stone
            {{ Tex::STONE, FaceType::PLUS_X_NORMAL },
             { Tex::STONE, FaceType::MINUS_X_NORMAL },
             { Tex::STONE, FaceType::PLUS_Z_NORMAL },
             { Tex::STONE, FaceType::MINUS_Z_NORMAL },
             { Tex::STONE, FaceType::PLUS_Y_NORMAL },
             { Tex::STONE, FaceType::MINUS_Y_NORMAL }},
            // grass plant
            {{ Tex::GRASS_PLANT, FaceType::MXMZ_TO_PXPZ_PLANT },
             { Tex::GRASS_PLANT, FaceType::PXPZ_TO_MXMZ_PLANT },
             { Tex::GRASS_PLANT, FaceType::MXPZ_TO_PXMZ_PLANT },
             { Tex::GRASS_PLANT, FaceType::PXMZ_TO_MXPZ_PLANT }},
            // Oak Log
            {{ Tex::OAK_LOG, FaceType::PLUS_X_NORMAL },
             { Tex::OAK_LOG, FaceType::MINUS_X_NORMAL },
             { Tex::OAK_LOG, FaceType::PLUS_Z_NORMAL },
             { Tex::OAK_LOG, FaceType::MINUS_Z_NORMAL },
             { Tex::OAK_LOG_END, FaceType::PLUS_Y_NORMAL },
             { Tex::OAK_LOG_END, FaceType::MINUS_Y_NORMAL }},
            // Oak Log PX
            {{ Tex::OAK_LOG_END, FaceType::PLUS_X_NORMAL },
             { Tex::OAK_LOG_END, FaceType::MINUS_X_NORMAL },
             { Tex::OAK_LOG, FaceType::PLUS_Z_NORMAL },
             { Tex::OAK_LOG, FaceType::MINUS_Z_NORMAL },
             { Tex::OAK_LOG, FaceType::PLUS_Y_NORMAL },
             { Tex::OAK_LOG, FaceType::MINUS_Y_NORMAL }},
            // Oak Log PZ
            {{ Tex::OAK_LOG, FaceType::PLUS_X_NORMAL },
             { Tex::OAK_LOG, FaceType::MINUS_X_NORMAL },
             { Tex::OAK_LOG_END, FaceType::PLUS_Z_NORMAL },
             { Tex::OAK_LOG_END, FaceType::MINUS_Z_NORMAL },
             { Tex::OAK_LOG, FaceType::PLUS_Y_NORMAL },
             { Tex::OAK_LOG, FaceType::MINUS_Y_NORMAL }},
            // Oak Leaves
            {{ Tex::OAK_LEAVES, FaceType::PLUS_X_NORMAL },
             { Tex::OAK_LEAVES, FaceType::MINUS_X_NORMAL },
             { Tex::OAK_LEAVES, FaceType::PLUS_Z_NORMAL },
             { Tex::OAK_LEAVES, FaceType::MINUS_Z_NORMAL },
             { Tex::OAK_LEAVES, FaceType::PLUS_Y_NORMAL },
             { Tex::OAK_LEAVES, FaceType::MINUS_Y_NORMAL }},
            // outline
            {{ Tex::OUTLINE, FaceType::PLUS_X_NORMAL },
             { Tex::OUTLINE, FaceType::MINUS_X_NORMAL },
             { Tex::OUTLINE, FaceType::PLUS_Z_NORMAL },
             { Tex::OUTLINE, FaceType::MINUS_Z_NORMAL },
             { Tex::OUTLINE, FaceType::PLUS_Y_NORMAL },
             { Tex::OUTLINE, FaceType::MINUS_Y_NORMAL }},
        };

        // For each vertex, store the light value, the offsets for the x, y,
        // and z positions, and the offsets for the x and y texture coordinates.
        // Index into this array with the FaceType enum.
        // Each face has 6 vertices, each with 6 attributes.
        // The first is a light value. For now, this is either 0 (-y face),
        // 1 (+z/-z face), 2 (+x/-x face) or 3 (+y face). In the future, this
        // value will be from 0-16 depending on how close it is to a light source.
        // The next 3 are the x, y, and z pixel offsets. These values are from
        // 0-48 and are the pixel offsets of the vertex. In some blocks, such as
        // crops and fences, the faces go outside the block. The larger range
        // allows for this. 16-32 is inside the block.
        // The last 2 values are the x and y texture offsets. These are always
        // either 0 or 1 ((0,0) is bottom left of texture, (1,1) is top right).
        vertex_attrib_t offs[NUM_FACE_TYPES][VERTICES_PER_FACE][6] = {
            // +x (normal)
            {{2, 32, 16, 32, 0, 0}, {2, 32, 16, 16, 1, 0}, {2, 32, 32, 16, 1, 1},
             {2, 32, 32, 16, 1, 1}, {2, 32, 32, 32, 0, 1}, {2, 32, 16, 32, 0, 0}},
            // -x (normal)
            {{2, 16, 16, 16, 0, 0}, {2, 16, 16, 32, 1, 0}, {2, 16, 32, 32, 1, 1},
             {2, 16, 32, 32, 1, 1}, {2, 16, 32, 16, 0, 1}, {2, 16, 16, 16, 0, 0}},
            // +z (normal)
            {{1, 16, 16, 32, 0, 0}, {1, 32, 16, 32, 1, 0}, {1, 32, 32, 32, 1, 1},
             {1, 32, 32, 32, 1, 1}, {1, 16, 32, 32, 0, 1}, {1, 16, 16, 32, 0, 0}},
            // -z (normal)
            {{1, 32, 16, 16, 0, 0}, {1, 16, 16, 16, 1, 0}, {1, 16, 32, 16, 1, 1},
             {1, 16, 32, 16, 1, 1}, {1, 32, 32, 16, 0, 1}, {1, 32, 16, 16, 0, 0}},
            // +y (normal)
            {{3, 16, 32, 32, 0, 0}, {3, 32, 32, 32, 1, 0}, {3, 32, 32, 16, 1, 1},
             {3, 32, 32, 16, 1, 1}, {3, 16, 32, 16, 0, 1}, {3, 16, 32, 32, 0, 0}},
            // -y (normal)
            {{0, 16, 16, 16, 0, 0}, {0, 32, 16, 16, 1, 0}, {0, 32, 16, 32, 1, 1},
             {0, 32, 16, 32, 1, 1}, {0, 16, 16, 32, 0, 1}, {0, 16, 16, 16, 0, 0}},
            // MXMZ_TO_PXPZ_PLANT
            {{3, 16, 16, 16, 0, 0}, {3, 32, 16, 32, 1, 0}, {3, 32, 32, 32, 1, 1},
             {3, 32, 32, 32, 1, 1}, {3, 16, 32, 16, 0, 1}, {3, 16, 16, 16, 0, 0}},
            // PXPZ_TO_MXMZ_PLANT
            {{3, 32, 16, 32, 0, 0}, {3, 16, 16, 16, 1, 0}, {3, 16, 32, 16, 1, 1},
             {3, 16, 32, 16, 1, 1}, {3, 32, 32, 32, 0, 1}, {3, 32, 16, 32, 0, 0}},
            // MXPZ_TO_PXMZ_PLANT
            {{3, 16, 16, 32, 0, 0}, {3, 32, 16, 16, 1, 0}, {3, 32, 32, 16, 1, 1},
             {3, 32, 32, 16, 1, 1}, {3, 16, 32, 32, 0, 1}, {3, 16, 16, 32, 0, 0}},
            // PXMZ_TO_MXPZ_PLANT
            {{3, 32, 16, 16, 0, 0}, {3, 16, 16, 32, 1, 0}, {3, 16, 32, 32, 1, 1},
             {3, 16, 32, 32, 1, 1}, {3, 32, 32, 16, 0, 1}, {3, 32, 16, 16, 0, 0}},
            // future: data for slabs, stairs, fences, torches, etc.
        };

        std::vector<Vertex> data;
        for (int f = 0; f < (int) blocks[(int) block].size(); ++f) {
            auto& [tex, face] = blocks[(int) block][f];
            auto& [texX, texY] = textures[(int) tex];
            for (int v = 0; v < VERTICES_PER_FACE; ++v) {
                Vertex vert = { 0, 0, 0 };
                vert.v2 = offs[(int) face][v][0] << 12; // light value
                vert.v2 += offs[(int) face][v][1] << 6; // x pixel position
                vert.v2 += offs[(int) face][v][2];      // y pixel position
                vert.v3 = offs[(int) face][v][3] << 10; // z pixel position
                vert.v3 += (offs[(int) face][v][4] + (vertex_attrib_t) texX) << 5; // x texture
                vert.v3 += offs[(int) face][v][5] + (vertex_attrib_t) texY;        // y texture
                data.push_back(vert);
            }
        }
        blockData[(int) block] = data;
    }

    void initBlockData() {
        for (int blockType = 0; blockType < NUM_BLOCK_TYPES; ++blockType) {
            setData(static_cast<BlockType>(blockType));
        }
    }

    // Return the number of vertex_attrib_t that have been added to data
    int getBlockData(BlockType type, int x, int y, int z, vertex_attrib_t* data,
                     const std::array<BlockType, NUM_DIRECTIONS>& surrounding) {
        assert(x >= 0 && y >= 0 && z >= 0);
        assert(x < CHUNK_WIDTH && z < CHUNK_WIDTH);
        assert(y < SUBCHUNK_HEIGHT);

        assert(isReal(type) || type == BlockType::OUTLINE);

        // position data: combine xyz coordinates into 16 bits
        vertex_attrib_t posData = (vertex_attrib_t) ((x << 12) + (y << 4) + z);

        int size = 0;
        std::vector<Vertex>& curBlockData = blockData[(int) type];
        int numFaces = (int) curBlockData.size() / VERTICES_PER_FACE;
        const std::vector<Direction>& d = DIR[(int) type];
        assert(numFaces == d.size());

        for (int face = 0; face < numFaces; ++face) {
            // If no direction (NO_DIR) is specified for this face, render the face.
            // If there is a solid block in the direction, don't render the face.
            if (d[face] != NO_DIR && isSolid(surrounding[d[face]])) {
                continue;
            }
            // retrieve the face's data
            for (int vert = 0; vert < VERTICES_PER_FACE; ++vert) {
                int index = face * VERTICES_PER_FACE + vert;
                data[size++] = curBlockData[index].v1 + posData;
                data[size++] = curBlockData[index].v2;
                data[size++] = curBlockData[index].v3;
            }
        }
        return size;
    }

    sglm::vec3 getBlockPosition(const Vertex& vertex) {
        float x = (float) (vertex.v1 >> 12);
        float y = (float) ((vertex.v1 >> 4) & 0xF);
        float z = (float) (vertex.v1 & 0xF);
        return { x, y, z };
    }

    sglm::vec3 getVertexPosition(const Vertex& vertex) {
        float x = (float((vertex.v2 >> 6) & 0x3F)  - 16.0f) / 16.0f;
        float y = (float(vertex.v2 & 0x3F)         - 16.0f) / 16.0f;
        float z = (float((vertex.v3 >> 10) & 0x3F) - 16.0f) / 16.0f;
        return getBlockPosition(vertex) + sglm::vec3(x, y, z);
    };
    
    // A block is real if it can appear in the world
    bool isReal(BlockType type) {
        switch (type) {
            case BlockType::AIR:
            case BlockType::GRASS:
            case BlockType::DIRT:
            case BlockType::STONE:
            case BlockType::GRASS_PLANT:
            case BlockType::OAK_LOG:
            case BlockType::OAK_LOG_PX:
            case BlockType::OAK_LOG_PZ:
            case BlockType::OAK_LEAVES:
                return true;
            case BlockType::OUTLINE:
            case BlockType::NO_BLOCK:
            case BlockType::NUM_BLOCK_TYPES:
                return false;
            default:
                std::cout << "Error: isReal(): blocktype not handled: " << ((int) type) << std::endl;
                throw type;
        }
    }

    // A block is normal if it has 6 faces with integer coordinates.
    // Non-normal blocks include plants, flowers, crops, slabs, stairs,
    // torches, chests, fences, signs, etc.
    bool isNormal(BlockType type) {
        switch (type) {
            case BlockType::AIR:
            case BlockType::GRASS_PLANT:
                return false;
            case BlockType::GRASS:
            case BlockType::DIRT:
            case BlockType::STONE:
            case BlockType::OAK_LOG:
            case BlockType::OAK_LOG_PX:
            case BlockType::OAK_LOG_PZ:
            case BlockType::OAK_LEAVES:
                return true;
            default:
                std::cout << "Error: isNormal(): blocktype not handled: " << ((int) type) << std::endl;
                throw type;
        }
    }

    // A block is solid if it is normal and not transparent
    bool isSolid(BlockType type) {
        switch (type) {
            case BlockType::GRASS:
            case BlockType::DIRT:
            case BlockType::STONE:
            case BlockType::OAK_LOG:
            case BlockType::OAK_LOG_PX:
            case BlockType::OAK_LOG_PZ:
            case BlockType::OAK_LEAVES:
                assert(isNormal(type));
                return true;
            case BlockType::AIR:
            case BlockType::GRASS_PLANT:
                assert(!isNormal(type));
            // treat NO_BLOCK as non-solid so that the top and bottom of the world are rendered
            case BlockType::NO_BLOCK:
                return false;
            default:
                std::cout << "Error: isSolid(): blocktype not handled: " << ((int) type) << std::endl;
                throw type;
        }
    }
}
