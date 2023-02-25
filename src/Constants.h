#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

#include <sglm/sglm.h>

enum Direction : unsigned char {
    PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y,
};

enum class Movement {
    FORWARD, BACKWARD, LEFT, RIGHT,
};

inline constexpr int INITIAL_SCREEN_WIDTH = 1200;
inline constexpr int INITIAL_SCREEN_HEIGHT = 800;
inline const char* WINDOW_TITLE = "Minecraft in OpenGL";

// Paths to resources from the solution directory
inline const char* BLOCK_VERTEX = "resources/shaders/block_vertex.glsl";
inline const char* BLOCK_FRAGMENT = "resources/shaders/block_fragment.glsl";
inline const char* UI_VERTEX = "resources/shaders/ui_vertex.glsl";
inline const char* UI_FRAGMENT = "resources/shaders/ui_fragment.glsl";
inline const char* TEXTURE_SHEET = "resources/textures/texture_sheet.png";

inline constexpr sglm::vec3 PLAYER_INITIAL_POSITION = { 0.0f, 80.0f, 0.0f };

// Determines how many chunks will load in each direction outward from the
// player. Chunks load in a a square around the chunk the player is in. In
// total there will be (LOAD_RADIUS * 2 + 1)^2 chunks loaded at a time.
inline constexpr int LOAD_RADIUS = 10;
inline constexpr int UNLOAD_RADIUS = LOAD_RADIUS + 2;

// This is about the distance from the center of a 16x16x16 sub-chunk to one
// of its corners. This value is used during frustum culling to determine
// whether a sub-chunk is within the view frustum. It is much easier to treat
// each sub-chaunk as a sphere than calculate its actual bounding box.
inline constexpr float SUB_CHUNK_RADIUS = 13.86f;

// Blocks less than PLAYER_REACH blocks away from the
// player can be mined and/or interacted with.
inline constexpr int PLAYER_REACH = 15;

// In order to render a block to the screen it must be broken up into vertices.
// Each of the 6 faces of a block are broken up into 2 triangles which have 3
// vertices each. Each vertex is encoded into a single unsigned int (see 
// BlockInfo.h and Chunk.cpp) before being sent to the shaders and GPU.
inline constexpr int FACES_PER_BLOCK = 6;
inline constexpr int VERTICES_PER_FACE = 6;
inline constexpr int VERTICES_PER_BLOCK = VERTICES_PER_FACE * FACES_PER_BLOCK;
inline constexpr int UINTS_PER_VERTEX = 1;
inline constexpr int UINTS_PER_FACE = VERTICES_PER_FACE * UINTS_PER_VERTEX;
inline constexpr int UINTS_PER_BLOCK = UINTS_PER_FACE * FACES_PER_BLOCK;
inline constexpr int BYTES_PER_FACE = UINTS_PER_FACE * sizeof(unsigned int);
inline constexpr int BYTES_PER_BLOCK = BYTES_PER_FACE * FACES_PER_BLOCK;

// Each chunk is a 16x128x16 section of the world. Dividing the world into
// chunks allows us to load only the portion of the world that is around the
// player while un-loading anything far away from the player.
inline constexpr int CHUNK_WIDTH = 16;   // x, z
inline constexpr int CHUNK_HEIGHT = 128; // y
inline constexpr int BLOCKS_PER_CHUNK = CHUNK_WIDTH * CHUNK_HEIGHT * CHUNK_WIDTH;

// Each chunk is divided into 16x16x16 sub-chunks. Dividing a chunk into
// sub-chunks allows us to update a block in the chunk without having to
// recreate the entire mesh for that chunk. Instead we would only have to
// recreate the mesh for the sub-chunk the block was in.
inline constexpr int SUBCHUNK_HEIGHT = 16;
inline constexpr int NUM_SUBCHUNKS = CHUNK_HEIGHT / SUBCHUNK_HEIGHT;
inline constexpr int BLOCKS_PER_SUBCHUNK = BLOCKS_PER_CHUNK / NUM_SUBCHUNKS;
inline constexpr int VERTICES_PER_SUBCHUNK = BLOCKS_PER_SUBCHUNK * VERTICES_PER_BLOCK;

inline constexpr sglm::vec3 WORLD_UP{ 0.0f, 1.0f, 0.0f };
inline constexpr float DEFAULT_YAW = -90.0f;
inline constexpr float DEFAULT_PITCH = 0.0f;
inline constexpr float DEFAULT_SPEED = 30.0f;
inline constexpr float DEFAULT_SENSITIVITY = 0.1f;

inline constexpr float DEFAULT_FOV = 60.0f;
inline constexpr float MIN_FOV = 5.0f;
inline constexpr float MAX_FOV = 90.0f;

inline constexpr float NEAR_PLANE = 0.1f;
inline constexpr float FAR_PLANE = 300.0f;

#endif
