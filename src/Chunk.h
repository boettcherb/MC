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

public:
    enum class Status : unsigned char {
        EMPTY,          // chunk is created, but nothing has been generated/loaded yet
        STRUCTURES,     // structures originating in this chunk have been created
        LOADING,        // This chunk is currently getting data from the db or from generateTerrain
        TERRAIN,        // This chunk's terrain has been generated (or loaded from db)
        FULL,           // This chunk is fully generated (has 4 neighbors with
                        // Status::TERRAIN and a mesh)
    };

private:
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
        int m_size;           // number of BlockTypes stored in this BlockList
        // TODO: remove
        bool m_built = false;

    public:
        BlockList();
        ~BlockList();

        Block::BlockType get(int x, int y, int z) const;
        void put(int x, int y, int z, Block::BlockType block);
        Block::BlockType* get_all() const;
        void create(const Block::BlockType* blocks, int size);
        void deleteAll();

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

        Subchunk(int y);
        void updateMesh(const Chunk* this_chunk);

    private:
        unsigned int getVertexData(const Chunk* this_chunk, int byte_lim,
                                   vertex_attrib_t* data) const;
    };

    const int m_X, m_Z;
    std::array<Subchunk*, NUM_SUBCHUNKS> m_subchunks;
    std::array<Chunk*, 4> m_neighbors;
    int m_numNeighbors;
    bool m_updated;
    Status m_status;
    bool m_toDelete; // true if block data should be deleted

public:
    Chunk(int x, int z);
    ~Chunk();

    Block::BlockType get(int x, int y, int z) const;
    void put(int x, int y, int z, Block::BlockType block);

    bool update();
    int render(Shader* shader, const sglm::frustum& frustum);

    void addBlockData(const Block::BlockType* blockData);
    void deleteBlockData();
    const void* getBlockData() const;

    void addNeighbor(Chunk* chunk, Direction direction);
    void removeNeighbor(Direction direction);
    std::pair<std::pair<int, int>, Chunk*> getNeighbor(int index) const;

    bool intersects(const sglm::ray& ray, Face::Intersection& isect);
    bool wasUpdated() const;
    void updateHandled();
    Status getStatus() const;
    void setLoading();
    void setToDelete();

    static void initNoise(); // in TerrainGen.cpp
    void generateTerrain(Block::BlockType* data, int seed) const; // in TerrainGen.cpp

    void generateStructures();

private:
    static int chunk_index(int x, int y, int z);
    static int subchunk_index(int x, int y, int z);
};

#endif
