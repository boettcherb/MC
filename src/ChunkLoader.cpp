#include "ChunkLoader.h"
#include "Chunk.h"
#include "Shader.h"
#include "Camera.h"
#include "math/sglm.h"
#include <new>
#include <map>
#include <cassert>

ChunkLoader::ChunkLoader(Shader* shader, int camX, int camZ) {
    for (int x = -CHUNK_LOAD_RADIUS; x <= CHUNK_LOAD_RADIUS; ++x) {
        for (int z = -CHUNK_LOAD_RADIUS; z <= CHUNK_LOAD_RADIUS; ++z) {
            addChunk(x, z);
        }
    }
    m_shader = shader;
    m_cameraX = camX;
    m_cameraZ = camZ;
}

ChunkLoader::~ChunkLoader() {
    for (const auto& itr : m_chunks) {
        delete itr.second;
    }
}

void ChunkLoader::update(const Camera* camera) {
    sglm::vec3 cameraPos = camera->getPosition();
    int camX = (int) cameraPos.x / CHUNK_LENGTH - (cameraPos.x < 0);
    int camZ = (int) cameraPos.z / CHUNK_WIDTH - (cameraPos.z < 0);
    if (camX != m_cameraX) {
        int oldX = m_cameraX + CHUNK_LOAD_RADIUS * (camX < m_cameraX ? 1 : -1);
        int newX = camX + CHUNK_LOAD_RADIUS * (camX < m_cameraX ? -1 : 1);
        for (int z = m_cameraZ - CHUNK_LOAD_RADIUS; z <= m_cameraZ + CHUNK_LOAD_RADIUS; ++z) {
            removeChunk(oldX, z);
            addChunk(newX, z);
        }
    }
    m_cameraX = camX;
    if (camZ != m_cameraZ) {
        int oldZ = m_cameraZ + CHUNK_LOAD_RADIUS * (camZ < m_cameraZ ? 1 : -1);
        int newZ = camZ + CHUNK_LOAD_RADIUS * (camZ < m_cameraZ ? -1 : 1);;
        for (int x = m_cameraX - CHUNK_LOAD_RADIUS; x <= m_cameraX + CHUNK_LOAD_RADIUS; ++x) {
            removeChunk(x, oldZ);
            addChunk(x, newZ);
        }
    }
    m_cameraZ = camZ;
}

void ChunkLoader::renderAll(const Camera& camera, float screenRatio) {
    for (const auto& itr : m_chunks) {
        itr.second->render(m_shader, camera.getViewMatrix(), camera.getZoom(), screenRatio);
    }
}

void ChunkLoader::addChunk(int x, int z) {
    assert(m_chunks.find({ x, z }) == m_chunks.end());
    Chunk* newChunk = new Chunk(x, z);
    auto px = m_chunks.find({ x + 1, z });
    auto mx = m_chunks.find({ x - 1, z });
    auto pz = m_chunks.find({ x, z + 1 });
    auto mz = m_chunks.find({ x, z - 1 });
    if (px != m_chunks.end()) {
        newChunk->addNeighbor(px->second, Chunk::Direction::PLUS_X);
        px->second->addNeighbor(newChunk, Chunk::Direction::MINUS_X);
    }
    if (mx != m_chunks.end()) {
        newChunk->addNeighbor(mx->second, Chunk::Direction::MINUS_X);
        mx->second->addNeighbor(newChunk, Chunk::Direction::PLUS_X);
    }
    if (pz != m_chunks.end()) {
        newChunk->addNeighbor(pz->second, Chunk::Direction::PLUS_Z);
        pz->second->addNeighbor(newChunk, Chunk::Direction::MINUS_Z);
    }
    if (mz != m_chunks.end()) {
        newChunk->addNeighbor(mz->second, Chunk::Direction::MINUS_Z);
        mz->second->addNeighbor(newChunk, Chunk::Direction::PLUS_Z);
    }
    m_chunks.emplace(std::make_pair(x, z), newChunk);
}

void ChunkLoader::removeChunk(int x, int z) {
    auto itr = m_chunks.find({ x, z });
    assert(itr != m_chunks.end());
    delete itr->second; // calls destructor, removes neighbors
    m_chunks.erase(itr);
}