#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include "Block.h"
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

    enum class Status {
        EMPTY, // chunk object is created, but nothing has been generated/loaded yet
        STRUCTURES, 
        // structures originating in this chunk have been created
        // has references to structures from all surrounding chunks (ready for terrain gen)

    };

    // Implementation in BlockList.cpp
    class BlockList {
        typedef unsigned long long uint64;

        std::vector<Block::BlockType> m_palette; // map from condensed id to block id
        std::array<int, (int) Block::BlockType::NUM_BLOCK_TYPES> m_index; // map from block id to condensed id
        uint64 m_bitmask;     // has m_bits_per_block least-significant bits set to 1
        uint64* m_data;       // stores the condensed block ids
        int m_data_size;      // the number of uint64s in m_data
        int m_bits_per_block; // number of bits used to represent each block
        int m_blocks_per_ll;  // number of blocks in each uint64 in m_data
        const int m_size;     // number of BlockTypes stored in this BlockList

    public:
        BlockList(const Block::BlockType* blocks, int size);
        ~BlockList();

        Block::BlockType get(int x, int y, int z) const;
        void put(int x, int y, int z, Block::BlockType block);
        Block::BlockType* get_all() const;

    private:
        void add_block(Block::BlockType block, bool rebuild);
        void build(const Block::BlockType* blocks);
        void fill_data(const Block::BlockType* blocks, int num_bits);
    };

    // Implementation in Subchunk.cpp
    struct Subchunk {
        const int m_Y;
        int m_mesh_size; // number of attributes
        Mesh m_mesh;
        BlockList m_blocks;


        Subchunk(int y, const Block::BlockType* data);
        void updateMesh(const Chunk* this_chunk);

    private:
        unsigned int getVertexData(const Chunk* this_chunk, int byte_lim,
                                   vertex_attrib_t* data) const;
    };

    const int m_posX, m_posZ;
    std::array<Subchunk*, NUM_SUBCHUNKS> m_subchunks;
    std::array<Chunk*, 4> m_neighbors;

    int m_numNeighbors;
    bool m_updated;  // true if any block has been updated since loading from db
    bool m_rendered; // true if meshes have been generated and this chunk is being rendered to the screen

public:
    Chunk(int x, int z, const Block::BlockType* blockData);
    ~Chunk();

    Block::BlockType get(int x, int y, int z) const;
    void put(int x, int y, int z, Block::BlockType block);
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
    void generateTerrain(Block::BlockType* data, int seed); // in TerrainGen.cpp
    static int chunk_index(int x, int y, int z);
    static int subchunk_index(int x, int y, int z);
};

#endif
