#include "Structure.h"

#include <random>
#include <cassert>

// mt is the engine which generates the random numbers
static std::mt19937 mt;

static std::uniform_int_distribution<std::mt19937::result_type> tree_height(6, 12);
static std::uniform_int_distribution<std::mt19937::result_type> giant_tree_height(20, 30);


static std::map<std::pair<int, int>, std::vector<Structure>> structures;

void Structure::create(StructureType type, int start_x, int start_z) {
    Structure s = Structure(type, start_x, start_z);
    for (auto itr = s.m_structure_blocks.begin(); itr != s.m_structure_blocks.end(); ++itr) {
        std::pair<int, int> key = itr->first;
        if (structures.find(key) == structures.end()) {
            structures[key] = std::vector<Structure>();
        }
        structures[key].push_back(s);
    }
}

Structure::Structure(StructureType type, int start_x, int start_z) :
m_startX{ start_x }, m_startZ{ start_z }, m_type{ type } {
    switch (type) {
        case StructureType::OAK_TREE:
            generateOakTree(start_x, start_z);
            break;
        case StructureType::JUNGLE_TREE:
            generateJungleTree(start_x, start_z);
            break;
        case StructureType::GIANT_JUNGLE_TREE:
            generateGiantJungleTree(start_x, start_z);
            break;
        case StructureType::JUNGLE_BUSH:
            generateJungleBush(start_x, start_z);
            break;
        default:
            throw "Unhandled Structure type";
    }
}

std::vector<Structure> Structure::getStructures(int cx, int cz) {
    if (structures.find({ cx, cz }) == structures.end()) {
        return { };
    }
    return structures[{ cx, cz }];
}

StructureType Structure::getType() const {
    return m_type;
}

std::pair<int, int> Structure::getStart() const {
    return { m_startX, m_startZ };
}

std::vector<s_block> Structure::getBlocks(int cx, int cz) const {
    std::pair<int, int> chunk = { cx, cz };
    if (m_structure_blocks.find(chunk) == m_structure_blocks.end()) {
        return { };
    }
    return m_structure_blocks.at(chunk);
}

static std::pair<int, int> getChunk(int x, int z) {
    int cx;
    if (x >= 0) {
        cx = x / CHUNK_WIDTH;
    } else {
        cx = x / CHUNK_WIDTH - 1;
        if (x % CHUNK_WIDTH == 0) {
            ++cx;
        }
    }
    int cz;
    if (z >= 0) {
        cz = z / CHUNK_WIDTH;
    } else {
        cz = z / CHUNK_WIDTH - 1;
        if (z % CHUNK_WIDTH == 0) {
            ++cz;
        }
    }
    return { cx, cz };
}

void Structure::addBlock(int x, int y, int z, Block::BlockType block) {
    std::pair<int, int> chunk = getChunk(x, z);
    if (m_structure_blocks.find(chunk) == m_structure_blocks.end()) {
        m_structure_blocks[chunk] = std::vector<s_block>();
    }
    x = x - chunk.first * CHUNK_WIDTH;
    z = z - chunk.second * CHUNK_WIDTH;
    assert(x >= 0 && x < CHUNK_WIDTH);
    assert(z >= 0 && z < CHUNK_WIDTH);
    m_structure_blocks[chunk].push_back({ x, y, z, block });
}

void Structure::generateOakTree(int start_x, int start_z) {
    mt.seed(start_x * start_z);
    int height = tree_height(mt);

    for (int i = 0; i < height; ++i) {
        addBlock(start_x, i, start_z, Block::BlockType::OAK_LOG);
    }
    
    Block::BlockType LEAF = Block::BlockType::OAK_LEAVES;
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (i == 0 && j == 0) {
                addBlock(start_x, height, start_z, LEAF);
                continue;
            }
            addBlock(start_x + i, height - 3, start_z + j, LEAF);
            addBlock(start_x + i, height - 2, start_z + j, LEAF);
            if (std::abs(i) < 2 && std::abs(j) < 2) {
                addBlock(start_x + i, height - 1, start_z + j, LEAF);
                if (std::abs(i + j) == 1) {
                    addBlock(start_x + i, height, start_z + j, LEAF);
                }
            }
        }
    }
}

void Structure::generateJungleTree(int start_x, int start_z) {
    mt.seed(start_x * start_z);
    int height = tree_height(mt);

    for (int i = 0; i < height; ++i) {
        addBlock(start_x, i, start_z, Block::BlockType::JUNGLE_LOG);
    }

    Block::BlockType LEAF = Block::BlockType::JUNGLE_LEAVES;
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (i == 0 && j == 0) {
                addBlock(start_x, height, start_z, LEAF);
                continue;
            }
            addBlock(start_x + i, height - 3, start_z + j, LEAF);
            addBlock(start_x + i, height - 2, start_z + j, LEAF);
            if (std::abs(i) < 2 && std::abs(j) < 2) {
                addBlock(start_x + i, height - 1, start_z + j, LEAF);
                if (std::abs(i + j) == 1) {
                    addBlock(start_x + i, height, start_z + j, LEAF);
                }
            }
        }
    }
}


void Structure::generateGiantJungleTree(int start_x, int start_z) {
    int height = giant_tree_height(mt);
    Block::BlockType WOOD = Block::BlockType::JUNGLE_LOG;
    Block::BlockType LEAF = Block::BlockType::JUNGLE_LEAVES;
    for (int y = 0; y <= height; ++y) {
        addBlock(start_x,     y, start_z,     WOOD);
        addBlock(start_x + 1, y, start_z,     WOOD);
        addBlock(start_x,     y, start_z + 1, WOOD);
        addBlock(start_x + 1, y, start_z + 1, WOOD);
    }
    for (int i = -8; i <= 8; ++i) {
        for (int j = -8; j <= 8; ++j) {
            int dist_sq = (i * i) + (j * j);
            if (dist_sq <= 36) {
                addBlock(start_x + i + 1, height - 1, start_z + j + 1, LEAF);
                addBlock(start_x + i + 1, height,     start_z + j + 1, LEAF);
                addBlock(start_x + i + 1, height + 1, start_z + j + 1, LEAF);
            }
            else if (dist_sq <= 49) {
                addBlock(start_x + i + 1, height - 1, start_z + j + 1, LEAF);
                addBlock(start_x + i + 1, height, start_z + j + 1, LEAF);
            }
            else if (dist_sq <= 64) {
                addBlock(start_x + i + 1, height - 1, start_z + j + 1, LEAF);
            }
        }
    }
}

void Structure::generateJungleBush(int start_x, int start_z) {
    addBlock(start_x, 0, start_z, Block::BlockType::JUNGLE_LOG);
    for (int i = -2; i <= 2; ++i) {
        for (int j = -2; j <= 2; ++j) {
            if (i == 0 && j == 0) {
                addBlock(start_x, 1, start_z, Block::BlockType::JUNGLE_LEAVES);
            }
            else if (i == -2 || j == -2 || i == 2 || j == 2) {
                addBlock(start_x + i, 0, start_z + j, Block::BlockType::JUNGLE_LEAVES);
            }
            else {
                addBlock(start_x + i, 0, start_z + j, Block::BlockType::JUNGLE_LEAVES);
                addBlock(start_x + i, 1, start_z + j, Block::BlockType::JUNGLE_LEAVES);
            }
        }
    }
}
