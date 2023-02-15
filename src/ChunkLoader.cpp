#include "ChunkLoader.h"
#include "Constants.h"
#include "Chunk.h"
#include "Shader.h"
#include "Camera.h"
#include <sglm/sglm.h>
#include <new>
#include <map>
#include <cassert>

ChunkLoader::ChunkLoader(Shader* shader, int camX, int camZ) {
    for (int x = -LOAD_RADIUS; x <= LOAD_RADIUS; ++x) {
        for (int z = -LOAD_RADIUS; z <= LOAD_RADIUS; ++z) {
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
    // Update which chunks are loaded if the camera crosses a chunk border
    sglm::vec3 cameraPos = camera->getPosition();
    int camX = (int) cameraPos.x / CHUNK_LENGTH - (cameraPos.x < 0);
    int camZ = (int) cameraPos.z / CHUNK_WIDTH - (cameraPos.z < 0);
    if (camX != m_cameraX) {
        int oldX = m_cameraX + LOAD_RADIUS * (camX < m_cameraX ? 1 : -1);
        int newX = camX + LOAD_RADIUS * (camX < m_cameraX ? -1 : 1);
        for (int z = m_cameraZ - LOAD_RADIUS; z <= m_cameraZ + LOAD_RADIUS; ++z) {
            removeChunk(oldX, z);
            addChunk(newX, z);
        }
    }
    m_cameraX = camX;
    if (camZ != m_cameraZ) {
        int oldZ = m_cameraZ + LOAD_RADIUS * (camZ < m_cameraZ ? 1 : -1);
        int newZ = camZ + LOAD_RADIUS * (camZ < m_cameraZ ? -1 : 1);;
        for (int x = m_cameraX - LOAD_RADIUS; x <= m_cameraX + LOAD_RADIUS; ++x) {
            removeChunk(x, oldZ);
            addChunk(x, newZ);
        }
    }
    m_cameraZ = camZ;

    // Update the view ray collision and block outline mesh
    Ray viewRay = Ray(camera->getPosition(), camera->getDirection());
    Face* bestFace = nullptr;
    int bestX = 0, bestZ = 0;
    for (int x = m_cameraX - 1; x <= m_cameraX + 1; ++x) {
        for (int z = m_cameraZ - 1; z <= m_cameraZ + 1; ++z) {
            Chunk* chunk = m_chunks.find({ x, z })->second;
            Face* face = chunk->findViewRayIntersection(viewRay);
            if (face != nullptr) {
                if (bestFace == nullptr || face->getT() < bestFace->getT()) {
                    bestFace = face;
                    bestX = x;
                    bestZ = z;
                }
            }
        }
    }
    if (bestFace == nullptr) {
        m_blockOutline.erase();
        m_outlineFace = nullptr;
    } else if (bestFace != m_outlineFace) {
        m_blockOutline.generate(VERTICES_PER_FACE, bestFace->getData(), false);
        m_outlineX = bestX;
        m_outlineZ = bestZ;
        m_outlineFace = bestFace;
    }
}

void ChunkLoader::renderAll(const Camera& camera, float screenRatio) {
    // send the view and projection matrices to the shader
    m_shader->addUniformMat4f("u1_view", camera.getViewMatrix());
    sglm::mat4 projection = sglm::perspective(sglm::radians(camera.getZoom()), screenRatio, 0.1f, 300.0f);
    m_shader->addUniformMat4f("u2_projection", projection);

    // render chunks
    for (const auto& itr : m_chunks) {
        itr.second->render(m_shader);
    }
    
    // render block outline
    if (m_blockOutline.generated()) {
        float x = (float) m_outlineX * CHUNK_LENGTH;
        float z = (float) m_outlineZ * CHUNK_WIDTH;
        sglm::vec3 translation = { x, 0.0f, z };
        m_shader->addUniformMat4f("u0_model", sglm::translate(translation));
        m_blockOutline.render(m_shader);
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

void ChunkLoader::removeChunk(int x, int z) {
    auto itr = m_chunks.find({ x, z });
    assert(itr != m_chunks.end());
    delete itr->second; // calls destructor, removes neighbors
    m_chunks.erase(itr);
}