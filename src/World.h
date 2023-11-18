#ifndef CHUNK_LOADER_H_INCLUDED
#define CHUNK_LOADER_H_INCLUDED

#include "Chunk.h"
#include "Shader.h"
#include "Player.h"
#include <sglm/sglm.h>
#include <map>

class World {
    std::map<std::pair<int, int>, Chunk*> m_chunks;
    Shader* m_shader;
    Player* m_player;

public:
    World(Shader* shader, Player* player);
    ~World();
    void update(bool mineBlock);
    void renderAll();

private:
    // void loadChunks(int camX, int camZ);
    // void checkViewRayCollisions();
    void addChunk(int x, int z, const void* data);
    void removeChunk(int x, int z);
};

#endif
