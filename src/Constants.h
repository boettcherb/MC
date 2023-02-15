#ifndef CONSTANTS_H_INCLUDED
#define CONSTANTS_H_INCLUDED

#include <sglm/sglm.h>

constexpr int LOAD_RADIUS = 2;
constexpr int PLAYER_REACH = 5;

constexpr int FACES_PER_BLOCK = 6;
constexpr int VERTICES_PER_FACE = 6;
constexpr int VERTICES_PER_BLOCK = VERTICES_PER_FACE * FACES_PER_BLOCK;
constexpr int UINTS_PER_VERTEX = 1;
constexpr int UINTS_PER_FACE = VERTICES_PER_FACE * UINTS_PER_VERTEX;
constexpr int BYTES_PER_FACE = UINTS_PER_FACE * sizeof(unsigned int);

constexpr int CHUNK_LENGTH = 16;  // x
constexpr int CHUNK_HEIGHT = 128; // y
constexpr int CHUNK_WIDTH = 16;   // z
constexpr int BLOCKS_PER_CHUNK = CHUNK_LENGTH * CHUNK_HEIGHT * CHUNK_WIDTH;

constexpr int MESH_HEIGHT = 16;
constexpr int NUM_MESHES = CHUNK_HEIGHT / MESH_HEIGHT;
constexpr int BLOCKS_PER_MESH = BLOCKS_PER_CHUNK / NUM_MESHES;
constexpr int VERTICES_PER_MESH = BLOCKS_PER_MESH * VERTICES_PER_BLOCK;

constexpr sglm::vec3 WORLD_UP{ 0.0f, 1.0f, 0.0f };
constexpr float DEFAULT_YAW = -90.0f;
constexpr float DEFAULT_PITCH = 0.0f;
constexpr float DEFAULT_SPEED = 60.0f;
constexpr float DEFAULT_SENSITIVITY = 0.1f;
constexpr float DEFAULT_ZOOM = 45.0f;

constexpr float MIN_FOV = 1.0f;
constexpr float MAX_FOV = 45.0f;

#endif
