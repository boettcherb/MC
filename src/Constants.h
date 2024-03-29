#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

#ifndef NDEBUG
#include <iostream>
#endif

enum Direction : unsigned char {
    PLUS_X, MINUS_X, PLUS_Z, MINUS_Z, PLUS_Y, MINUS_Y, NUM_DIRECTIONS
};

enum class Movement {
    FORWARD, BACKWARD, LEFT, RIGHT,
};

inline constexpr int INITIAL_SCREEN_WIDTH = 800;
inline constexpr int INITIAL_SCREEN_HEIGHT = 800;
inline const char* WINDOW_TITLE = "Minecraft in OpenGL";

// Paths to resources from the solution directory
inline const char* BLOCK_VERTEX = "resources/shaders/block_vertex.glsl";
inline const char* BLOCK_FRAGMENT = "resources/shaders/block_fragment.glsl";
inline const char* UI_VERTEX = "resources/shaders/ui_vertex.glsl";
inline const char* UI_FRAGMENT = "resources/shaders/ui_fragment.glsl";
inline const char* TEXTURE_SHEET = "resources/textures/texture_sheet.png";

// This is about the distance from the center of a 16x16x16 sub-chunk to one
// of its corners. This value is used during frustum culling to determine
// whether a sub-chunk is within the view frustum. It is much easier to treat
// each sub-chaunk as a sphere than calculate its actual bounding box.
inline constexpr float SUB_CHUNK_RADIUS = 13.86f;

typedef unsigned short VertexAttribType;
struct Vertex {
    VertexAttribType v1, v2, v3;
};
inline constexpr int ATTRIBS_PER_VERTEX = 3;
inline constexpr int VERTICES_PER_FACE = 6;
inline constexpr int ATTRIBS_PER_FACE = ATTRIBS_PER_VERTEX * VERTICES_PER_FACE;
inline constexpr int VERTEX_SIZE = sizeof(VertexAttribType) * ATTRIBS_PER_VERTEX;

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

inline constexpr float NEAR_PLANE = 0.1f;
inline constexpr float FAR_PLANE = 600.0f;

#endif
