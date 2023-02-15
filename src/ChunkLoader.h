#ifndef CHUNK_LOADER_H_INCLUDED
#define CHUNK_LOADER_H_INCLUDED

#include "Chunk.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include <math/sglm.h>
#include <map>

constexpr int CHUNK_LOAD_RADIUS = 2;

class ChunkLoader {
    std::map<std::pair<int, int>, Chunk*> m_chunks;
    Shader* m_shader;
    int m_cameraX, m_cameraZ; // x and z of chunk that camera is in

    Mesh m_blockOutline;
    int m_outlineX, m_outlineZ; // x and z of chunk that has the block outline mesh
    Face* m_outlineFace;

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
