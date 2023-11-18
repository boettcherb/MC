#include "World.h"
#include "Constants.h"
#include "Chunk.h"
#include "Shader.h"
#include "Player.h"
#include "Face.h"
#include "Database.h"

#include <new>
#include <map>
#include <cassert>
#include <thread>
#include <mutex>
#include <chrono>

#ifdef NDEBUG
#define SGLM_NO_PRINT
#endif
#define SGLM_IMPLEMENTATION
#include <sglm/sglm.h>

World::World(Shader* shader, Player* player) {
    auto [cx, cz] = player->getPlayerChunk();
    for (int x = cx - 2; x <= cx + 2; ++x) {
        for (int z = cz - 2; z <= cz + 2; ++z) {
            database::request_load(x, z);
        }
    }
    m_shader = shader;
    m_player = player;
    m_chunkLoaderThreadShouldClose = false;
    m_chunkLoaderThread = std::thread(&World::chunkLoaderThreadFunc, this);
}

World::~World() {
    m_chunkLoaderThreadShouldClose = true;
    m_chunkLoaderThread.join();
    assert(m_frontier.empty());
    assert(m_chunks.empty());
}

// called once every frame
// mineBlock: true if the player has pressed the left mouse button. If the
// player is looking at a block, it will be mined.
void World::update(bool mineBlock) {
    m_chunksMutex.lock();
    for (const auto& [_, chunk] : m_chunks) {
        chunk->update();
    }
    m_chunksMutex.unlock();

    checkViewRayCollisions();

    // mine block we are looking at
    if (mineBlock && m_player->hasViewRayIsect()) {
        const Face::Intersection& isect = m_player->getViewRayIsect();
        m_chunksMutex.lock();
        Chunk* chunk = m_chunks.find({ isect.cx, isect.cz })->second;
        m_chunksMutex.unlock();
        chunk->put(isect.x, isect.y, isect.z, Block::BlockType::AIR, true);
    }
}

// determine if the player is looking at a block (if yes, we
// want to render a block outline around that block 
void World::checkViewRayCollisions() {
    sglm::ray viewRay = { m_player->getPosition(), 
        m_player->getDirection(), (float) Player::getReach() };
    bool foundIntersection = false;
    Face::Intersection bestI = Face::Intersection();
    // loop through chunks near the player (the player's
    // view distance is < width of 1 chunk)
    auto [cx, cz] = m_player->getPlayerChunk();
    m_chunksMutex.lock();
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
    m_chunksMutex.unlock();
    if (foundIntersection) {
        bestI.setData();
        m_player->setViewRayIsect(&bestI);
    } else {
        m_player->setViewRayIsect(nullptr);
    }
}

void World::renderAll() {
    // send the view and projection matrices to the shader
    m_shader->addUniformMat4f("u1_view", m_player->getViewMatrix());
    m_shader->addUniformMat4f("u2_projection", m_player->getProjectionMatrix());

    // render block outline
    if (m_player->hasViewRayIsect()) {
        const Face::Intersection& isect = m_player->getViewRayIsect();
        float x = (float) (isect.cx * CHUNK_WIDTH);
        float z = (float) (isect.cz * CHUNK_WIDTH);
        m_shader->addUniformMat4f("u0_model", sglm::translate({ x, 0.0f, z }));
        m_player->renderOutline(m_shader);
    }
    
    // render chunks
    int rendered = 0, total = 0;
    m_chunksMutex.lock();
    for (const auto& [_, chunk] : m_chunks) {
        rendered += chunk->render(m_shader, m_player->getFrustum());
        total += NUM_SUBCHUNKS;
    }
    m_chunksMutex.unlock();
    Player::chunks_rendered = { rendered, total };
}

void World::chunkLoaderThreadFunc() {
    using namespace std::chrono_literals;
    while (!m_chunkLoaderThreadShouldClose) {

    // load chunks from the database
        database::Query q = database::get_load_result();
        while (q.type != database::QUERY_NONE) {
            assert(q.type == database::QUERY_LOAD);
            threadAddChunk(q.x, q.z, q.data);
            delete[] q.data;
            q = database::get_load_result();
        }

        std::this_thread::sleep_for(500ms);

    }

    // thread is closing, unload all chunks
    while (!m_frontier.empty()) {
        auto itr = m_frontier.begin();
        int x = itr->first.first;
        int z = itr->first.second;
        Chunk* chunk = itr->second;
        threadRemoveChunk(x, z, chunk);
    }
    assert(m_frontier.empty());
    assert(m_chunks.empty());
}

void World::threadAddChunk(int x, int z, const void* data) {
    m_chunksMutex.lock();
   // if the chunk has already been loaded, don't do anything
    if (m_chunks.count({ x, z }) == 1 || m_frontier.count({ x, z }) == 1) {
        m_chunksMutex.unlock();
        return;
    }

    // none of the neighbors should be in m_chunks
    assert(m_chunks.count({ x + 1, z }) == 0);
    assert(m_chunks.count({ x - 1, z }) == 0);
    assert(m_chunks.count({ x, z + 1 }) == 0);
    assert(m_chunks.count({ x, z - 1 }) == 0);

    // at least one of the neighbors should be absent from the frontier (not loaded)
    assert(m_frontier.find({ x + 1, z }) == m_frontier.end()
           || m_frontier.find({ x - 1, z }) == m_frontier.end()
           || m_frontier.find({ x, z + 1 }) == m_frontier.end()
           || m_frontier.find({ x, z - 1 }) == m_frontier.end());

    // create the new chunk and add it to the frontier
    // unlock while generating terrain for the chunk
    m_chunksMutex.unlock();
    Chunk* newChunk = new Chunk(x, z, data);
    m_chunksMutex.lock();
    m_frontier.emplace(std::make_pair(x, z), newChunk);

    // add neighbors to the new chunk. If the new chunk's neighbors have
    // all 4 neighbors, remove them from the frontier and add them to m_chunks.
    auto px = m_frontier.find({ x + 1, z });
    if (px != m_frontier.end()) {
        newChunk->addNeighbor(px->second, PLUS_X);
        px->second->addNeighbor(newChunk, MINUS_X);
        if (px->second->getNumNeighbors() == 4) {
            m_chunks.emplace(std::make_pair(x + 1, z), px->second);
            m_frontier.erase(px);
        }
    }
    auto mx = m_frontier.find({ x - 1, z });
    if (mx != m_frontier.end()) {
        newChunk->addNeighbor(mx->second, MINUS_X);
        mx->second->addNeighbor(newChunk, PLUS_X);
        if (mx->second->getNumNeighbors() == 4) {
            m_chunks.emplace(std::make_pair(x - 1, z), mx->second);
            m_frontier.erase(mx);
        }
    }
    auto pz = m_frontier.find({ x, z + 1 });
    if (pz != m_frontier.end()) {
        newChunk->addNeighbor(pz->second, PLUS_Z);
        pz->second->addNeighbor(newChunk, MINUS_Z);
        if (pz->second->getNumNeighbors() == 4) {
            m_chunks.emplace(std::make_pair(x, z + 1), pz->second);
            m_frontier.erase(pz);
        }
    }
    auto mz = m_frontier.find({ x, z - 1 });
    if (mz != m_frontier.end()) {
        newChunk->addNeighbor(mz->second, MINUS_Z);
        mz->second->addNeighbor(newChunk, PLUS_Z);
        if (mz->second->getNumNeighbors() == 4) {
            m_chunks.emplace(std::make_pair(x, z - 1), mz->second);
            m_frontier.erase(mz);
        }
    }

    m_chunksMutex.unlock();
}

void World::threadRemoveChunk(int x, int z, Chunk* chunk) {
    m_chunksMutex.lock();
    assert(m_frontier.count({ x, z }) == 1);
    m_frontier.erase({ x, z });
    // Add all neighbors to the frontier (if they aren't already there)
    for (int i = 0; i < 4; ++i) {
        auto [pos, neighbor] = chunk->getNeighbor(i);
        // if the neighbor is already in the frontier, don't do anything
        if (neighbor == nullptr || m_frontier.count(pos) == 1)
            continue;
        // if the neighbor is not in the frontier, then they are in m_chunks
        assert(m_chunks.count(pos) == 1);
        // add them to the frontier and remove them from m_chunks
        m_frontier.emplace(pos, neighbor);
        m_chunks.erase(pos);
    }
    m_chunksMutex.unlock();
    if (chunk->wasUpdated()) {
        database::request_store(x, z, BLOCKS_PER_CHUNK, chunk->getBlockData());
    }
    delete chunk; // calls destructor, removes neighbors
}
