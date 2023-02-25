#ifndef CHUNK_LOADER_H_INCLUDED
#define CHUNK_LOADER_H_INCLUDED

#include "Chunk.h"
#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "Face.h"
#include <sglm/sglm.h>
#include <map>

class ChunkLoader {
    std::map<std::pair<int, int>, Chunk*> m_chunks;
    Shader* m_shader;
    int m_cameraX, m_cameraZ; // x and z of chunk that camera is in

    Mesh m_blockOutline;
    int m_outlineX, m_outlineZ; // x and z of chunk that has the block outline mesh
    Face::Intersection m_viewRayIsect;

public:
    ChunkLoader(Shader* shader, int camX, int camZ);
    ~ChunkLoader();
    void update(const Camera& camera, bool mineBlock);
    void renderAll(const Camera& camera);

private:
    void loadChunks(int camX, int camZ);
    void checkViewRayCollisions(const Camera& camera);
    void addChunk(int x, int z, const void* data);
    void removeChunk(int x, int z);
};

#endif
