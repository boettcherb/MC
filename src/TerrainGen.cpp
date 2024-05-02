#include "Chunk.h"
#include "Block.h"
#include "Structure.h"
#include <FastNoiseLite/FastNoiseLite.h>

#include <random>
#include <vector>
#include <tuple>
#include <cmath>
#include <cassert>
#include <iostream>

enum class Biome {
    DESERT, JUNGLE, FOREST, PLAINS, TUNDRA,
};

static FastNoiseLite terrain_height; // simplex noise that determines the ground height
static FastNoiseLite biome; // cellular noise that determines the biome
static FastNoiseLite noise3d; // simplex 3d noise used for cave generation

// mt is the engine which generates the random numbers
static std::mt19937 mt;

static std::uniform_int_distribution<std::mt19937::result_type> forest_noise(1, 300);
static std::uniform_int_distribution<std::mt19937::result_type> plains_noise(1, 1500);
static std::uniform_int_distribution<std::mt19937::result_type> desert_noise(1, 80);
static std::uniform_int_distribution<std::mt19937::result_type> jungle_noise(1, 600);

constexpr int WATER_HEIGHT = 35;

void Chunk::initNoise() {
    noise3d.SetSeed(1337);
    noise3d.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise3d.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise3d.SetFrequency(0.008f);

    terrain_height.SetSeed(1337);
    terrain_height.SetFractalOctaves(5);
    terrain_height.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    terrain_height.SetFractalType(FastNoiseLite::FractalType_FBm);
    terrain_height.SetFrequency(0.003f);

    biome.SetSeed(1337);
    biome.SetFrequency(0.007f);
    biome.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    biome.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Hybrid);
    biome.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
    biome.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    biome.SetDomainWarpAmp(300.0f);
    biome.SetFractalType(FastNoiseLite::FractalType_DomainWarpIndependent);
}

double map(double x, double in_min, double in_max, double out_min, double out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int getHeight(int x, int z) {
    double h = terrain_height.GetNoise((double) x, (double) z);
    return (int) map(h, -1, 1, 20, 90);
}

Biome getBiome(int x, int z) {
    double b = biome.GetNoise((double) x, (double) z);
    if (b < -0.8)
        return Biome::DESERT;
    else if (b < -0.4)
        return Biome::JUNGLE;
    else if (b < 0.2)
        return Biome::FOREST;
    else if (b < 0.6)
        return Biome::PLAINS;
    else
        return Biome::TUNDRA;
}

// fill data (a 1D array of BLOCKS_PER_CHUNK blocks) with
// the type of each block in the chunk
void Chunk::generateTerrain(Block::BlockType* data, int seed) const {
    mt.seed(seed ^ (m_X + 100000) ^ (m_Z + 100000));

    std::fill(data, data + BLOCKS_PER_CHUNK, Block::BlockType::AIR);

    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            int nx = x + CHUNK_WIDTH * m_X;
            int nz = z + CHUNK_WIDTH * m_Z;
            int height = getHeight(nx, nz);
            for (int y = 0; y <= height; ++y) {
                // double a = y < height - 5 ? 0.05 : 0.00;
                // if (abs(noise3d.GetNoise((double) nx, (double) y, (double) nz)) > a) {
                data[Chunk::chunk_index(x, y, z)] = Block::BlockType::STONE;
            // }
            }
            if (height <= WATER_HEIGHT) {
                for (int y = height; y <= WATER_HEIGHT; ++y) {
                    data[Chunk::chunk_index(x, y, z)] = Block::BlockType::WATER;
                }
                continue;
            }
            data[Chunk::chunk_index(x, height - 3, z)] = Block::BlockType::DIRT;
            data[Chunk::chunk_index(x, height - 2, z)] = Block::BlockType::DIRT;
            data[Chunk::chunk_index(x, height - 1, z)] = Block::BlockType::DIRT;
            data[Chunk::chunk_index(x, height, z)] = Block::BlockType::GRASS;
            Biome b = getBiome(nx, nz);
            int val;
            switch (b) {
                case Biome::DESERT:
                    data[Chunk::chunk_index(x, height - 3, z)] = Block::BlockType::SAND;
                    data[Chunk::chunk_index(x, height - 2, z)] = Block::BlockType::SAND;
                    data[Chunk::chunk_index(x, height - 1, z)] = Block::BlockType::SAND;
                    data[Chunk::chunk_index(x, height, z)] = Block::BlockType::SAND;
                    val = desert_noise(mt);
                    if (val == 1) {
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::CACTUS;
                        data[Chunk::chunk_index(x, height + 2, z)] = Block::BlockType::CACTUS;
                        data[Chunk::chunk_index(x, height + 3, z)] = Block::BlockType::CACTUS;
                        data[Chunk::chunk_index(x, height + 4, z)] = Block::BlockType::CACTUS;
                    } else if (val < 5) {
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::DEAD_BUSH;
                    }
                    break;
                case Biome::FOREST:
                    val = forest_noise(mt);
                    if (val < 50)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::GRASS_PLANT;
                    else if (val < 51)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::BLUE_FLOWER;
                    else if (val < 52)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::PINK_FLOWER;
                    else if (val < 53)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::RED_FLOWER;
                    break;
                case Biome::PLAINS:
                    val = plains_noise(mt);
                    if (val < 300)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::GRASS_PLANT;
                    else if (val < 350)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::BLUE_FLOWER;
                    else if (val < 400)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::PINK_FLOWER;
                    else if (val < 450)
                        data[Chunk::chunk_index(x, height + 1, z)] = Block::BlockType::RED_FLOWER;
                    break;
                case Biome::TUNDRA:
                    data[Chunk::chunk_index(x, height - 3, z)] = Block::BlockType::SNOW;
                    data[Chunk::chunk_index(x, height - 2, z)] = Block::BlockType::SNOW;
                    data[Chunk::chunk_index(x, height - 1, z)] = Block::BlockType::SNOW;
                    data[Chunk::chunk_index(x, height, z)] = Block::BlockType::SNOW;
                    break;
            }
        }
    }
    std::vector<Structure> structures = Structure::getStructures(m_X, m_Z);
    for (const Structure& s : structures) {
        std::vector<s_block> structure_blocks = s.getBlocks(m_X, m_Z);
        std::pair<int, int> start = s.getStart();
        int height = getHeight(start.first, start.second);
        for (const s_block& sb : structure_blocks) {
            const auto& [x, y, z, block] = sb;
            if (s.getType() == StructureType::JUNGLE_BUSH) {
                int X = m_X * CHUNK_WIDTH + x;
                int Z = m_Z * CHUNK_WIDTH + z;
                height = getHeight(X, Z);
            }
            data[Chunk::chunk_index(x, y + height, z)] = block;
        }
    }
}

void Chunk::generateStructures() {
    mt.seed(1337 ^ (m_X + 100000) ^ (m_Z + 100000));
    for (int x = 0; x < CHUNK_WIDTH; ++x) {
        for (int z = 0; z < CHUNK_WIDTH; ++z) {
            int X = m_X * CHUNK_WIDTH + x;
            int Z = m_Z * CHUNK_WIDTH + z;
            Biome b = getBiome(X, Z);
            int height = getHeight(X, Z);
            if (height <= WATER_HEIGHT)
                continue;
            if (b == Biome::JUNGLE) {
                int val = jungle_noise(mt);
                if (val < 2) {
                    Structure::create(StructureType::GIANT_JUNGLE_TREE, X, Z);
                } else  if (val < 8) {
                    Structure::create(StructureType::JUNGLE_TREE, X, Z);
                } else if (val < 64) {
                    Structure::create(StructureType::JUNGLE_BUSH, X, Z);
                }
            }
            else if (b == Biome::FOREST) {
                if (forest_noise(mt) <= 10) {
                    Structure::create(StructureType::OAK_TREE, X, Z);
                }
            }
            else if (b == Biome::PLAINS) {
                if (plains_noise(mt) == 1) {
                    Structure::create(StructureType::OAK_TREE, X, Z);
                }
            }
        }
    }
    m_status = Status::STRUCTURES;
}
