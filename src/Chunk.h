#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include "BlockInfo.h"
#include "Constants.h"
#include "Shader.h"
#include "Mesh.h"
#include "Face.h"

#include <vector>
#include <tuple>

// Each chunk is a 16x128x16 section of the world. All the blocks of a chunk
// are generated, loaded, and stored together. Each chunk is divided into 8
// 16x16x16 meshes.

typedef std::vector<std::tuple<int, int, int, Block::BlockType>> BlockList;

class Chunk {
    const int m_posX, m_posZ;
    Block::BlockType m_blockArray[CHUNK_WIDTH][CHUNK_HEIGHT][CHUNK_WIDTH];
    Mesh m_mesh[NUM_SUBCHUNKS];
    Chunk* m_neighbors[4];
    int m_numNeighbors;
    bool m_updated;
    bool m_rendered; // true if meshes have been generated

    // stores blocks that were generated in the terrain generator but fall
    // outside this chunk. For example, if a tree is generated on the chunk
    // boundary, some of its leaves may be in a neighboring chunk. That
    // neighboring chunk may not be loaded yet, so store those blocks here
    // until the neighbor is loaded.
    BlockList m_outsideBlocks;

public:
    Chunk(int x, int z, const void* blockData);
    ~Chunk();

    Block::BlockType get(int x, int y, int z) const;
    void put(int x, int y, int z, Block::BlockType block, bool updateMesh);

    bool update();
    int render(Shader* shader, const sglm::frustum& frustum);

    void addNeighbor(Chunk* chunk, Direction direction);
    void removeNeighbor(Direction direction);
    std::pair<std::pair<int, int>, Chunk*> getNeighbor(int index) const;
    int getNumNeighbors() const;

    bool intersects(const sglm::ray& ray, Face::Intersection& isect);
    const void* getBlockData() const;
    bool wasUpdated() const;

    static void initNoise(); // in TerrainGen.cpp

private:
    void updateMesh(int meshIndex);
    unsigned int getVertexData(VertexAttribType* data, int meshIndex) const;

    void generateTerrain(int seed); // in TerrainGen.cpp

};

#endif
