#include "Chunk.h"
#include <FastNoiseLite/FastNoiseLite.h>

#include <random>
#include <vector>
#include <tuple>
#include <cassert>

enum class Biome {
    FOREST, PLAINS, OCEAN, DESERT, JUNGLE, TUNDRA,
};

static FastNoiseLite terrain_height; // simplex noise that determines the ground height
static FastNoiseLite biome; // cellular noise that determines the biome
// create a uniform int distribution that produces ints from 0 to 99 inclusive
// mt is the engine which generates the random numbers
static std::mt19937 mt;
static std::uniform_int_distribution<std::mt19937::result_type> r(0, 99);
static std::uniform_int_distribution<std::mt19937::result_type> tree_height(4, 8);


void Chunk::initNoise() {
    terrain_height.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    terrain_height.SetFractalType(FastNoiseLite::FractalType_FBm);
    terrain_height.SetFrequency(0.003f);

    biome.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    biome.SetFractalType(FastNoiseLite::FractalType_DomainWarpIndependent);
    biome.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
    biome.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    biome.SetCellularJitter(1.0f);
    biome.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    biome.SetDomainWarpAmp(300.0f);
}

static int getHeight(float x, float z) {
    float h = terrain_height.GetNoise(x, z);
    float min = 30.0f, max = 80.0f;
    return (int) ((h + 1.0) / 2.0 * (max - min) + min);
}

static Biome getBiome(float x, float z) {
    biome.SetFrequency(0.01f);
    biome.DomainWarp(x, z);
    biome.SetFrequency(0.002f);
    float n = biome.GetNoise(x, z);
    float cutoffs[] = { -0.7f, -0.4f, -0.1f, 0.2f, 0.6f, 1.0f };
    for (int i = 0; i < sizeof(cutoffs) / sizeof(float); ++i) {
        if (n < cutoffs[i]) {
            return static_cast<Biome>(i);
        }
    }
    throw "Invalid biome";
}

static void addTree(int x, int y, int z, BlockList& blocks) {
    int height = tree_height(mt);
    assert(height >= 4 && height <= 8);

    for (int i = -1; i < height; ++i) {
        blocks.emplace_back(x, y + i, z, Block::BlockType::OAK_LOG);
    }

    Block::BlockType LEAF = Block::BlockType::OAK_LEAVES;
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (i == 0 && j == 0) {
                blocks.emplace_back(x, y + height, z, LEAF);
                continue;
            }
            blocks.emplace_back(x + i, y + height - 3, z + j, LEAF);
            blocks.emplace_back(x + i, y + height - 2, z + j, LEAF);
            if (std::abs(i) < 2 && std::abs(j) < 2) {
                blocks.emplace_back(x + i, y + height - 1, z + j, LEAF);
                if (std::abs(i + j) == 1) {
                    blocks.emplace_back(x + i, y + height, z + j, LEAF);
                }
            }
        }
    }
}

void Chunk::generateTerrain(int seed) {
    BlockList blocks;
    blocks.reserve(BLOCKS_PER_CHUNK);

    mt.seed(seed * m_posX * m_posZ);
    terrain_height.SetSeed(seed);
    biome.SetSeed(seed);

    std::fill(&m_blockArray[0][0][0], &m_blockArray[0][0][0] + BLOCKS_PER_CHUNK, Block::BlockType::AIR);

    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            float nx = (float) x + CHUNK_WIDTH * m_posX;
            float nz = (float) z + CHUNK_WIDTH * m_posZ;
            int groundHeight = getHeight(nx, nz);
            for (int y = 0; y <= groundHeight - 4; ++y) {
                blocks.emplace_back(x, y, z, Block::BlockType::STONE);
            }
            blocks.emplace_back(x, groundHeight - 3, z, Block::BlockType::DIRT);
            blocks.emplace_back(x, groundHeight - 2, z, Block::BlockType::DIRT);
            blocks.emplace_back(x, groundHeight - 1, z, Block::BlockType::DIRT);
            blocks.emplace_back(x, groundHeight, z, Block::BlockType::GRASS);
            Biome b = getBiome(nx, nz);
            if (b == Biome::FOREST || b == Biome::PLAINS || b == Biome::OCEAN) {
                blocks.emplace_back(x, groundHeight + 1, z, Block::BlockType::GRASS_PLANT);
            } else if (x == 8 && z == 8) {
                addTree(x, groundHeight + 1, z, blocks);
            }
        }
    }

    for (std::tuple<int, int, int, Block::BlockType> t : blocks) {
        auto& [x, y, z, block] = t;
        put(x, y, z, block, false);
    }
}
