#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include "BlockInfo.h"
#include "Constants.h"
#include "Shader.h"
#include "Mesh.h"
#include "Face.h"

#include <vector>
#include <array>

// Each chunk is a 16x128x16 section of the world. All the blocks of a chunk
// are generated, loaded, and stored together. Each chunk is divided into 8
// 16x16x16 meshes.

class Chunk {

    // Implementation in BlockList.cpp
    class BlockList {
        typedef unsigned long long uint64;

        std::vector<Block::BlockType> m_palette; // map from condensed id to block id
        std::array<int, (int) Block::BlockType::NUM_BLOCK_TYPES> m_index; // map from block id to condensed id
        unsigned int* m_data;
        uint64 m_bitmask;
        int m_bits_per_block;
        int m_size;

    public:
        BlockList(const Block::BlockType* blocks, int size);
        ~BlockList();

        Block::BlockType get(int x, int y, int z) const;
        void put(int x, int y, int z, Block::BlockType block);
        Block::BlockType* get_all() const;

    private:
        std::pair<uint64, int> get_data(int block_index) const;
        void add_block(Block::BlockType block, bool rebuild);
        void build(const Block::BlockType* blocks);
        void fill_data(const Block::BlockType* blocks, int num_bits);
    };

    const int m_posX, m_posZ;
    BlockList* m_blocks;
    Mesh m_mesh[NUM_SUBCHUNKS];
    std::array<Chunk*, 4> m_neighbors;
    unsigned char m_highest_solid_block[CHUNK_WIDTH][CHUNK_WIDTH];
    int m_numNeighbors;
    bool m_updated;  // true if any block has been updated since loading from db
    bool m_rendered; // true if meshes have been generated and this chunk is being rendered to the screen

public:
    Chunk(int x, int z, const Block::BlockType* blockData);
    ~Chunk();

    Block::BlockType get(int x, int y, int z) const;
    void put(int x, int y, int z, Block::BlockType block, bool updateMesh = false);
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
    void generateTerrain(Block::BlockType* data, int seed); // in TerrainGen.cpp
    static int index(int x, int y, int z);
};

#endif
