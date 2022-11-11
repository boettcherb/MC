#ifndef CHUNK_H_INCLUDED
#define CHUNK_H_INCLUDED

#include "BlockInfo.h"
#include "ShaderProgram.h"
#include "Mesh.h"

constexpr int CHUNK_LENGTH = 16;  // x
constexpr int CHUNK_HEIGHT = 128; // y
constexpr int CHUNK_WIDTH = 16;  // z
constexpr int BLOCKS_PER_CHUNK = CHUNK_LENGTH * CHUNK_HEIGHT * CHUNK_WIDTH;

class Chunk {

    struct Blocks {
        Blocks() = default;
        Block::BlockType m_blockArray[CHUNK_LENGTH][CHUNK_HEIGHT][CHUNK_WIDTH];
    };

    const float m_posX, m_posZ;
    Blocks* m_blocks;
    Mesh* m_mesh;
    ShaderProgram* m_shader;
    Chunk* m_neighbors[4];

public:
    enum Direction : unsigned char {
        PLUS_X, MINUS_X, PLUS_Z, MINUS_Z
    };

    Chunk(float x, float z, ShaderProgram* shader);
    ~Chunk();

    void put(int x, int y, int z, Block::BlockType block);
    Block::BlockType get(int x, int y, int z) const;
    void updateMesh();
    void render(glm::mat4 viewMatrix, float zoom, float scrRatio);
    void addNeighbor(Chunk* chunk, Direction direction);

private:
    void generateTerrain();
    unsigned int getVertexData(unsigned int* data) const;
    inline void setBlockFaceData(unsigned int* data, int x, int y, int z, const unsigned int* blockData) const;
};

#endif
