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

World::World(Shader* shader, Player* player) {
    auto [cx, cz] = player->getPlayerChunk();
    for (int x = cx - LOAD_RADIUS; x <= cx + LOAD_RADIUS; ++x) {
        for (int z = cz - LOAD_RADIUS; z <= cz + LOAD_RADIUS; ++z) {
            database::request_load(x, z);
        }
    }
    m_shader = shader;
    m_player = player;
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
    static int prevX = camX;
    static int prevZ = camZ;
    assert(std::abs(camX - prevX) <= 1);
    assert(std::abs(camZ - prevZ) <= 1);
    if (camX != prevX) {
        int x = camX + (camX < prevX ? -LOAD_RADIUS : LOAD_RADIUS);
        for (int z = prevZ - LOAD_RADIUS; z <= prevZ + LOAD_RADIUS; ++z) {
            database::request_load(x, z);
        }
        x = prevX + (camX < prevX ? UNLOAD_RADIUS : -UNLOAD_RADIUS);
        for (int z = prevZ - UNLOAD_RADIUS; z <= prevZ + UNLOAD_RADIUS; ++z) {
            removeChunk(x, z);
        }
        prevX = camX;
    }
    if (camZ != prevZ) {
        int z = camZ + (camZ < prevZ ? -LOAD_RADIUS : LOAD_RADIUS);
        for (int x = prevX - LOAD_RADIUS; x <= prevX + LOAD_RADIUS; ++x) {
            database::request_load(x, z);
        }
        z = prevZ + (camZ < prevZ ? UNLOAD_RADIUS : -UNLOAD_RADIUS);
        for (int x = prevX - UNLOAD_RADIUS; x <= prevX + UNLOAD_RADIUS; ++x) {
            removeChunk(x, z);
        }
        prevZ = camZ;
    }
}

// called once every frame
// mineBlock: true if the player has pressed the left mouse button. If the
// player is looking at a block, it will be mined.
void World::update(bool mineBlock) {
    // each chunk, load at most 1 chunk from the database
    database::Query q = database::get_load_result();
    if (q.type != database::QUERY_NONE) {
        assert(q.type == database::QUERY_LOAD);
        addChunk(q.x, q.z, q.data);
        delete[] q.data;
    }
    
    // each frame, find the chunk the camera is in and compare it to the
    // camera's position on the previous frame. If the positions are
    // different, load/unload some chunks
    // TODO: create another thread to take care of chunk loading/unloading
    std::pair<int, int> player_chunk = m_player->getPlayerChunk();
    loadChunks(player_chunk.first, player_chunk.second);

    checkViewRayCollisions();

    // mine block we are looking at
    if (mineBlock && m_player->hasViewRayIsect()) {
        const Face::Intersection& isect = m_player->getViewRayIsect();
        Chunk* chunk = m_chunks.find({ isect.cx, isect.cz })->second;
        chunk->put(isect.x, isect.y, isect.z, Block::BlockType::AIR, true);
    }
}

// determine if the player is looking at a block (if yes, we
// want to render a block outline around that block 
void World::checkViewRayCollisions() {
    const Camera& cam = m_player->getCamera();
    sglm::ray viewRay = { cam.getPosition(), cam.getDirection() };
    bool foundIntersection = false;
    Face::Intersection bestI = Face::Intersection();
    // loop through chunks near the player (the player's
    // view distance is < width of 1 chunk)
    auto [cx, cz] = m_player->getPlayerChunk();
    for (int x = cx - 1; x <= cx + 1; ++x) {
        for (int z = cz - 1; z <= cz + 1; ++z) {
            auto itr = m_chunks.find({ x, z });
            if (itr == m_chunks.end())
                continue;
            Chunk* chunk = itr->second;
            // chunk->intersects returns whether there was an intersection).
            // If yes, the details of that intersection are stored in i.
            Face::Intersection i;
            if (chunk->intersects(viewRay, i)) {
                if (!foundIntersection || i.t < bestI.t) {
                    foundIntersection = true;
                    bestI = i;
                    bestI.cx = x;
                    bestI.cz = z;
                }
            }
        }
    }
    if (foundIntersection) {
        bestI.setData();
        m_player->setViewRayIsect(&bestI);
    } else {
        m_player->setViewRayIsect(nullptr);
    }
}

void World::renderAll() {
    const Camera& cam = m_player->getCamera();
    // send the view and projection matrices to the shader
    m_shader->addUniformMat4f("u1_view", cam.getViewMatrix());
    m_shader->addUniformMat4f("u2_projection", cam.getProjectionMatrix());

    // render block outline
    if (m_player->hasViewRayIsect()) {
        const Face::Intersection& isect = m_player->getViewRayIsect();
        float x = (float) (isect.cx * CHUNK_WIDTH);
        float z = (float) (isect.cz * CHUNK_WIDTH);
        m_shader->addUniformMat4f("u0_model", sglm::translate({ x, 0.0f, z }));
        m_player->renderOutline(m_shader);
    }
    
    // render chunks
    for (const auto& itr : m_chunks) {
        itr.second->render(m_shader, cam.getFrustum());
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
