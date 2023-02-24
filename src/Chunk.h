#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include "BlockInfo.h"
#include "Constants.h"
#include "Shader.h"
#include "Mesh.h"
#include "Face.h"

// Each chunk is a 16x128x16 section of the world. All the blocks of a chunk
// are generated, loaded, and stored together. Each chunk is divided into 8
// 16x16x16 meshes.

class Chunk {
    const int m_posX, m_posZ;
    Block::BlockType m_blockArray[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_WIDTH];
    Mesh m_mesh[NUM_SUBCHUNKS];
    Chunk* m_neighbors[4];
    int m_numNeighbors;
    bool m_updated;

public:
    Chunk(int x, int z);
    ~Chunk();

    Block::BlockType get(int x, int y, int z) const;
    void put(int x, int y, int z, Block::BlockType block, bool updateMesh);
    void render(Shader* shader, const sglm::frustum& frustum);
    void addNeighbor(Chunk* chunk, Direction direction);
    void removeNeighbor(Direction direction);
    bool intersects(const sglm::ray& ray, Face::Intersection& isect);

private:
    void updateMesh(int meshIndex);
    void generateTerrain();
    unsigned int getVertexData(unsigned int* data, int meshIndex) const;
};

#endif
