#ifndef CHUNK_LOADER_H_INCLUDED
#define CHUNK_LOADER_H_INCLUDED

#include "Chunk.h"
#include "Shader.h"
#include "Player.h"
#include <sglm/sglm.h>
#include <map>
#include <mutex>
#include <thread>

class World {
    std::map<std::pair<int, int>, Chunk*> m_chunks;
    Shader* m_shader;
    Player* m_player;

    bool m_chunkLoaderThreadShouldClose;
    std::thread m_chunkLoaderThread;
    std::mutex m_chunksMutex;

public:
    World(Shader* shader, Player* player);
    ~World();
    void update(bool mineBlock);
    void renderAll();

private:
    void checkViewRayCollisions();

    void LoadChunks();
    void addChunk(int x, int z, const void* data);
    void removeChunk(int x, int z, Chunk* chunk);
};

#endif
