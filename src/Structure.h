#ifndef STRUCTURE_H_INCLUDED
#define STRUCTURE_H_INCLUDED

#include "Block.h"

#include <map>
#include <vector>
#include <tuple>

enum class StructureType {
    OAK_TREE, JUNGLE_TREE, GIANT_JUNGLE_TREE, JUNGLE_BUSH
};

typedef std::tuple<int, int, int, Block::BlockType> s_block;

class Structure {
    StructureType m_type;
    int m_startX, m_startZ;
    std::map<std::pair<int, int>, std::vector<s_block>> m_structure_blocks;

public:
    static std::vector<Structure> getStructures(int cx, int cz);
    static void create(StructureType type, int start_x, int start_z);
    StructureType getType() const;
    std::pair<int, int> getStart() const;
    std::vector<s_block> getBlocks(int cx, int cz) const;

private:
    Structure(StructureType type, int start_x, int start_z);
    void addBlock(int x, int y, int z, Block::BlockType block);
    void generateOakTree(int start_x, int start_z);
    void generateJungleTree(int start_x, int start_z);
    void generateGiantJungleTree(int start_x, int start_z);
    void generateJungleBush(int start_x, int start_z);

};

#endif
