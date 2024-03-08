#include "Chunk.h"
#include <FastNoise/FastNoise.h>

void Chunk::generateTerrain() {
    FastNoise noise;
    for (int X = 0; X < CHUNK_WIDTH; ++X) {
        for (int Z = 0; Z < CHUNK_WIDTH; ++Z) {
            float noiseX = (float) X + CHUNK_WIDTH * m_posX;
            float noiseZ = (float) Z + CHUNK_WIDTH * m_posZ;
            int groundHeight = (int) ((noise.GetSimplexFractal(noiseX, noiseZ) + 1.0) / 2.0 * 50.0 + 50);
            for (int Y = 0; Y <= groundHeight - 4; ++Y) {
                put(X, Y, Z, Block::BlockType::STONE, false);
            }
            put(X, groundHeight - 3, Z, Block::BlockType::DIRT, false);
            put(X, groundHeight - 2, Z, Block::BlockType::DIRT, false);
            put(X, groundHeight - 1, Z, Block::BlockType::DIRT, false);
            put(X, groundHeight, Z, Block::BlockType::GRASS, false);
            for (int Y = groundHeight + 1; Y < CHUNK_HEIGHT; ++Y) {
                put(X, Y, Z, Block::BlockType::AIR, false);
            }
        }
    }
}