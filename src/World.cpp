#include "World.h"
#include "Constants.h"
#include "Chunk.h"
#include "Shader.h"
#include "Camera.h"
#include "Face.h"
#include "Database.h"
#include <new>
#include <map>
#include <cassert>

#ifdef NDEBUG
#define SGLM_NO_PRINT
#endif
#define SGLM_IMPLEMENTATION
#include <sglm/sglm.h>

World::World(Shader* shader, int camX, int camZ) {
    for (int x = camX - LOAD_RADIUS; x <= camX + LOAD_RADIUS; ++x) {
        for (int z = camZ - LOAD_RADIUS; z <= camZ + LOAD_RADIUS; ++z) {
            database::request_load(x, z);
        }
    }
    m_shader = shader;
    m_cameraX = camX;
    m_cameraZ = camZ;
    m_outlineX = m_outlineZ = 0;
    m_viewRayIsect = Face::Intersection();
}

World::~World() {
    while (!m_chunks.empty()) {
        auto itr = m_chunks.begin();
        int x = itr->first.first;
        int z = itr->first.second;
        removeChunk(x, z);
    }
}

void World::loadChunks(int camX, int camZ) {
    if (camX != m_cameraX) {
        int x = camX + (camX < m_cameraX ? -LOAD_RADIUS : LOAD_RADIUS);
        for (int z = m_cameraZ - LOAD_RADIUS; z <= m_cameraZ + LOAD_RADIUS; ++z) {
            database::request_load(x, z);
        }
        x = m_cameraX + (camX < m_cameraX ? UNLOAD_RADIUS : -UNLOAD_RADIUS);
        for (int z = m_cameraZ - UNLOAD_RADIUS; z <= m_cameraZ + UNLOAD_RADIUS; ++z) {
            removeChunk(x, z);
        }
        m_cameraX = camX;
    }
    if (camZ != m_cameraZ) {
        int z = camZ + (camZ < m_cameraZ ? -LOAD_RADIUS : LOAD_RADIUS);
        for (int x = m_cameraX - LOAD_RADIUS; x <= m_cameraX + LOAD_RADIUS; ++x) {
            database::request_load(x, z);
        }
        z = m_cameraZ + (camZ < m_cameraZ ? UNLOAD_RADIUS : -UNLOAD_RADIUS);
        for (int x = m_cameraX - UNLOAD_RADIUS; x <= m_cameraX + UNLOAD_RADIUS; ++x) {
            removeChunk(x, z);
        }
        m_cameraZ = camZ;
    }
}

void World::update(const Camera& camera, bool mineBlock) {
    database::Query q = database::get_load_result();
    if (q.type != database::QUERY_NONE) {
        assert(q.type == database::QUERY_LOAD);
        addChunk(q.x, q.z, q.data);
        delete[] q.data;
    }
    
    sglm::vec3 cameraPos = camera.getPosition();
    int camX = (int) cameraPos.x / CHUNK_WIDTH - (cameraPos.x < 0);
    int camZ = (int) cameraPos.z / CHUNK_WIDTH - (cameraPos.z < 0);
    assert(std::abs(camX - m_cameraX) <= 1);
    assert(std::abs(camZ - m_cameraZ) <= 1);
    loadChunks(camX, camZ);

    checkViewRayCollisions(camera);

    // mine block we are looking at
    if (mineBlock && m_viewRayIsect.x != -1) {
        Chunk* chunk = m_chunks.find({ m_outlineX, m_outlineZ })->second;
        int x = m_viewRayIsect.x;
        int y = m_viewRayIsect.y;
        int z = m_viewRayIsect.z;
        chunk->put(x, y, z, Block::BlockType::AIR, true);
    }
}

void World::checkViewRayCollisions(const Camera& camera) {
    sglm::ray viewRay = { camera.getPosition(), camera.getDirection() };
    bool foundIntersection = false;
    Face::Intersection bestI = Face::Intersection(), i;
    int bestX = 0, bestZ = 0;
    for (int x = m_cameraX - 1; x <= m_cameraX + 1; ++x) {
        for (int z = m_cameraZ - 1; z <= m_cameraZ + 1; ++z) {
            auto itr = m_chunks.find({ x, z });
            if (itr == m_chunks.end()) {
                continue;
            }
            Chunk* chunk = itr->second;
            if (chunk->intersects(viewRay, i)) {
                if (!foundIntersection || i.t < bestI.t) {
                    foundIntersection = true;
                    bestI = i;
                    bestX = x;
                    bestZ = z;
                }
            }
        }
    }
    if (foundIntersection) {
        bestI.setData();
        if (!(bestI == m_viewRayIsect)) {
            m_blockOutline.generate(BYTES_PER_BLOCK, bestI.data, false);
            m_outlineX = bestX;
            m_outlineZ = bestZ;
            m_viewRayIsect = bestI;
        }
    } else {
        m_blockOutline.erase();
        m_viewRayIsect.x = m_viewRayIsect.y = m_viewRayIsect.z = -1;
    }
}

void World::renderAll(const Camera& camera) {
    // send the view and projection matrices to the shader
    m_shader->addUniformMat4f("u1_view", camera.getViewMatrix());
    m_shader->addUniformMat4f("u2_projection", camera.getProjectionMatrix());

    // render block outline
    if (m_blockOutline.generated()) {
        float x = (float) (m_outlineX * CHUNK_WIDTH);
        float z = (float) (m_outlineZ * CHUNK_WIDTH);
        m_shader->addUniformMat4f("u0_model", sglm::translate({ x, 0.0f, z }));
        m_blockOutline.render(m_shader);
    }
    
    // render chunks
    for (const auto& itr : m_chunks) {
        itr.second->render(m_shader, camera.getFrustum());
    }
}

void World::addChunk(int x, int z, const void* data) {
    if (m_chunks.find({ x, z }) != m_chunks.end()) {
        return;
    }
    Chunk* newChunk = new Chunk(x, z, data);
    auto px = m_chunks.find({ x + 1, z });
    auto mx = m_chunks.find({ x - 1, z });
    auto pz = m_chunks.find({ x, z + 1 });
    auto mz = m_chunks.find({ x, z - 1 });
    if (px != m_chunks.end()) {
        newChunk->addNeighbor(px->second, PLUS_X);
        px->second->addNeighbor(newChunk, MINUS_X);
    }
    if (mx != m_chunks.end()) {
        newChunk->addNeighbor(mx->second, MINUS_X);
        mx->second->addNeighbor(newChunk, PLUS_X);
    }
    if (pz != m_chunks.end()) {
        newChunk->addNeighbor(pz->second, PLUS_Z);
        pz->second->addNeighbor(newChunk, MINUS_Z);
    }
    if (mz != m_chunks.end()) {
        newChunk->addNeighbor(mz->second, MINUS_Z);
        mz->second->addNeighbor(newChunk, PLUS_Z);
    }
    m_chunks.emplace(std::make_pair(x, z), newChunk);
}

void World::removeChunk(int x, int z) {
    auto itr = m_chunks.find({ x, z });
    if (itr != m_chunks.end()) {
        Chunk* chunk = itr->second;
        if (chunk->wasUpdated()) {
            database::request_store(x, z, BLOCKS_PER_CHUNK, chunk->getBlockData());
        }
        delete itr->second; // calls destructor, removes neighbors
        m_chunks.erase(itr);
    }
}
