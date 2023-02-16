#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

#include <sglm/sglm.h>

enum Direction : unsigned char {
    PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y,
};

enum class Movement {
    FORWARD, BACKWARD, LEFT, RIGHT,
};

// Determines how many chunks will load in each direction outward from the
// player. Chunks load in a a square around the chunk the player is in. In
// total there will be (LOAD_RADIUS * 2 + 1)^2 chunks loaded at a time.
inline constexpr int LOAD_RADIUS = 2;

// Blocks less than PLAYER_REACH blocks away from the
// player can be mined and/or interacted with.
inline constexpr int PLAYER_REACH = 5;

// In order to render a block to the screen it must be broken up into vertices.
// Each of the 6 faces of a block are broken up into 2 triangles which have 3
// vertices each. Each vertex is encoded into a single unsigned int (see 
// BlockInfo.h and Chunk.cpp) before being sent to the shaders and GPU.
inline constexpr int FACES_PER_BLOCK = 6;
inline constexpr int VERTICES_PER_FACE = 6;
inline constexpr int VERTICES_PER_BLOCK = VERTICES_PER_FACE * FACES_PER_BLOCK;
inline constexpr int UINTS_PER_VERTEX = 1;
inline constexpr int UINTS_PER_FACE = VERTICES_PER_FACE * UINTS_PER_VERTEX;
inline constexpr int BYTES_PER_FACE = UINTS_PER_FACE * sizeof(unsigned int);

// Each chunk is a 16x128x16 section of the world. Dividing the world into
// chunks allows us to load only the portion of the world that is around the
// player while un-loading anything far away from the player.
inline constexpr int CHUNK_LENGTH = 16;  // x
inline constexpr int CHUNK_HEIGHT = 128; // y
inline constexpr int CHUNK_WIDTH = 16;   // z
inline constexpr int BLOCKS_PER_CHUNK = CHUNK_LENGTH * CHUNK_HEIGHT * CHUNK_WIDTH;

// Each chunk is divided into 16x16x16 sub-chunks. Dividing a chunk into
// sub-chunks allows us to update a block in the chunk without having to
// recreate the entire mesh for that chunk. Instead we would only have to
// recreate the mesh for the sub-chunk the block was in.
inline constexpr int SUBCHUNK_HEIGHT = 16;
inline constexpr int NUM_SUBCHUNKS = CHUNK_HEIGHT / SUBCHUNK_HEIGHT;
inline constexpr int BLOCKS_PER_SUBCHUNK = BLOCKS_PER_CHUNK / NUM_SUBCHUNKS;
inline constexpr int VERTICES_PER_SUBCHUNK = BLOCKS_PER_SUBCHUNK * VERTICES_PER_BLOCK;

#endif
