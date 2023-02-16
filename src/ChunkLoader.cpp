#include "ChunkLoader.h"
#include "Constants.h"
#include "Chunk.h"
#include "Shader.h"
#include "Camera.h"
#include <sglm/sglm.h>
#include <new>
#include <map>
#include <cassert>










// #include <iostream>
// #include <bitset>










ChunkLoader::ChunkLoader(Shader* shader, int camX, int camZ) {
    for (int x = camX - LOAD_RADIUS; x <= camX + LOAD_RADIUS; ++x) {
        for (int z = camZ - LOAD_RADIUS; z <= camZ + LOAD_RADIUS; ++z) {
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
        assert(std::abs(camX - m_cameraX) == 1);
        int oldX = m_cameraX + LOAD_RADIUS * (camX < m_cameraX ? 1 : -1);
        int newX = camX + LOAD_RADIUS * (camX < m_cameraX ? -1 : 1);
        for (int z = m_cameraZ - LOAD_RADIUS; z <= m_cameraZ + LOAD_RADIUS; ++z) {
            removeChunk(oldX, z);
            addChunk(newX, z);
        }
    }
    m_cameraX = camX;
    if (camZ != m_cameraZ) {
        assert(std::abs(camZ - m_cameraZ) == 1);
        int oldZ = m_cameraZ + LOAD_RADIUS * (camZ < m_cameraZ ? 1 : -1);
        int newZ = camZ + LOAD_RADIUS * (camZ < m_cameraZ ? -1 : 1);;
        for (int x = m_cameraX - LOAD_RADIUS; x <= m_cameraX + LOAD_RADIUS; ++x) {
            removeChunk(x, oldZ);
            addChunk(x, newZ);
        }
    }
    m_cameraZ = camZ;

    // Update the view ray collision and block outline mesh



    sglm::ray viewRay = { camera->getPosition(), camera->getDirection() };

    // std::cout << "camera position: (" << camera->getPosition().x << ", " << camera->getPosition().z << ")\n";
    // std::cout << "camera chunk: (" << m_cameraX << ", " << m_cameraZ << ")\n";
    // std::cout << "viewRay pos: (" << viewRay.pos.x << ", " << viewRay.pos.y << ", " << viewRay.pos.z << ")\n";
    // std::cout << "viewRay dir: (" << viewRay.dir.x << ", " << viewRay.dir.y << ", " << viewRay.dir.z << ")\n";

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
        // std::cout << "NO COLLISION\n";
        m_blockOutline.erase();
        m_outlineFace = nullptr;
    } else if (bestFace != m_outlineFace) {
        m_blockOutline.generate(BYTES_PER_FACE, bestFace->getData(), false);
        m_outlineX = bestX;
        m_outlineZ = bestZ;
        m_outlineFace = bestFace;
        // std::cout << "COLLISION: bestX: " << m_outlineX << ", bestZ: " << m_outlineZ << std::endl;
        // std::cout << "    Face:";
        // sglm::vec3 A = bestFace->A, B = bestFace->B, C = bestFace->C, D = bestFace->D, normal = bestFace->normal;
        // std::cout << " (" << A.x << ", " << A.y << ", " << A.z << ")";
        // std::cout << " (" << B.x << ", " << B.y << ", " << B.z << ")"; 
        // std::cout << " (" << C.x << ", " << C.y << ", " << C.z << ")";
        // std::cout << " (" << D.x << ", " << D.y << ", " << D.z << ")" << std::endl;
        // std::cout << "    Normal: " << normal.x << ", " << normal.y << ", " << normal.z << ")\n";
        // std::bitset<32> abit(bestFace->getData()[0]);
        // std::bitset<32> bbit(bestFace->getData()[1]);
        // std::bitset<32> cbit(bestFace->getData()[2]);
        // std::bitset<32> dbit(bestFace->getData()[4]);
        // std::cout << std::hex << "A: " << abit << '\n';
        // std::cout << std::hex << "B: " << bbit << '\n';
        // std::cout << std::hex << "C: " << cbit << '\n';
        // std::cout << std::hex << "D: " << dbit << '\n';
        // std::cout << std::dec << '\n';
    }

    // std::cout << "\n========================================\n\n";
}

void ChunkLoader::renderAll(const Camera& camera, float screenRatio) {
    // send the view and projection matrices to the shader
    m_shader->addUniformMat4f("u1_view", camera.getViewMatrix());
    sglm::mat4 projection = sglm::perspective(sglm::radians(camera.getZoom()), screenRatio, 0.1f, 300.0f);
    m_shader->addUniformMat4f("u2_projection", projection);

    // render block outline
    if (m_blockOutline.generated()) {
        float x = (float) m_outlineX * CHUNK_LENGTH;
        float z = (float) m_outlineZ * CHUNK_WIDTH;
        sglm::vec3 translation = { x, 0.0f, z };
        m_shader->addUniformMat4f("u0_model", sglm::translate(translation));
        m_blockOutline.render(m_shader);
    }
    
    // render chunks
    for (const auto& itr : m_chunks) {
        itr.second->render(m_shader);
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