#ifndef CHUNK_LOADER_H_INCLUDED
#define CHUNK_LOADER_H_INCLUDED

#include "Chunk.h"
#include "Shader.h"
#include "Camera.h"
#include "math/sglm.h"
#include <map>

constexpr int CHUNK_LOAD_RADIUS = 20;

class ChunkLoader {
    std::map<std::pair<int, int>, Chunk*> m_chunks;
    Shader* m_shader;
    int m_cameraX, m_cameraZ; // x and z of chunk that camera is in

public:
    ChunkLoader(Shader* shader, int camX, int camZ);
    ~ChunkLoader();
    void update(const Camera* camera);
    void renderAll(const Camera& camera, float screenRatio);

private:
    void addChunk(int x, int z);
    void removeChunk(int x, int z);
};

#endif
