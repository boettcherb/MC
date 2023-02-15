#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include "BlockInfo.h"
#include "Shader.h"
#include "Mesh.h"
#include <math/Face.h>

// Each chunk is a 16x128x16 section of the world. All the blocks of a chunk
// are generated, loaded, and stored together. Each chunk is divided into 8
// 16x16x16 meshes.

constexpr int CHUNK_LENGTH = 16;  // x
constexpr int CHUNK_HEIGHT = 128; // y
constexpr int CHUNK_WIDTH = 16;  // z
constexpr int BLOCKS_PER_CHUNK = CHUNK_LENGTH * CHUNK_HEIGHT * CHUNK_WIDTH;
constexpr int MESH_HEIGHT = 16;
constexpr int NUM_MESHES = CHUNK_HEIGHT / MESH_HEIGHT;
constexpr int BLOCKS_PER_MESH = BLOCKS_PER_CHUNK / NUM_MESHES;

class Chunk {
    const int m_posX, m_posZ;
    Block::BlockType m_blockArray[CHUNK_LENGTH][CHUNK_HEIGHT][CHUNK_WIDTH];
    Mesh m_mesh[NUM_MESHES];
    Chunk* m_neighbors[4];
    int m_numNeighbors;

public:
    enum Direction : unsigned char {
        PLUS_X, MINUS_X, PLUS_Z, MINUS_Z
    };

    Chunk(int x, int z);
    ~Chunk();

    void put(int x, int y, int z, Block::BlockType block);
    Block::BlockType get(int x, int y, int z) const;
    void updateMesh();
    void render(Shader* shader);
    void addNeighbor(Chunk* chunk, Direction direction);
    void removeNeighbor(Direction direction);
    Face* findViewRayIntersection(const Ray& ray);

private:
    void generateTerrain();
    unsigned int getVertexData(unsigned int* data, int meshIndex) const;
    inline void setBlockFaceData(unsigned int* data, int x, int y, int z, const unsigned int* blockData) const;
};

#endif
