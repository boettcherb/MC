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
#include <set>
#include <vector>

#ifdef NDEBUG
#define SGLM_NO_PRINT
#endif
#define SGLM_IMPLEMENTATION
#include <sglm/sglm.h>

World::World(Shader* shader, Player* player) {
    // initially, load a 5x5 grid of chunks around the player
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
    assert(m_chunks.empty());
}

// called once every frame
// mineBlock: true if the player has pressed the left mouse button. If the
// player is looking at a block, it will be mined.
void World::update(bool /* mineBlock */) {
    m_chunksMutex.lock();
    for (const auto& [_, chunk] : m_chunks) {
        chunk->update();
    }
    m_chunksMutex.unlock();
    /*
    checkViewRayCollisions();

    // mine block we are looking at
    if (mineBlock && m_player->hasViewRayIsect()) {
        const Face::Intersection& isect = m_player->getViewRayIsect();
        m_chunksMutex.lock();
        Chunk* chunk = m_chunks.find({ isect.cx, isect.cz })->second;
        m_chunksMutex.unlock();
        chunk->put(isect.x, isect.y, isect.z, Block::BlockType::AIR, true);
    }
    */
}

// determine if the player is looking at a block (if yes, we
// want to render a block outline around that block
/*
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
*/

void World::renderAll() {
    // send the view and projection matrices to the shader
    m_shader->addUniformMat4f("u1_view", m_player->getViewMatrix());
    m_shader->addUniformMat4f("u2_projection", m_player->getProjectionMatrix());
    /*
    // render block outline
    if (m_player->hasViewRayIsect()) {
        const Face::Intersection& isect = m_player->getViewRayIsect();
        float x = (float) (isect.cx * CHUNK_WIDTH);
        float z = (float) (isect.cz * CHUNK_WIDTH);
        m_shader->addUniformMat4f("u0_model", sglm::translate({ x, 0.0f, z }));
        m_player->renderOutline(m_shader);
    }
    */
    
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
        /*
        // create a copy of the chunks so I can loop through it many times
        // without having to worry about holding the mutex too long.
        std::vector<std::pair<std::pair<int, int>, Chunk*>> chunks;
        m_chunksMutex.lock();
        chunks.reserve(m_chunks.size());
        for (const auto& [pos, chunk] : m_chunks) {
            chunks.push_back({ pos, chunk });
        }
        m_chunksMutex.unlock();

        // Find all unloaded chunks that are adjacent to loaded chunks. Add
        // these chunks to the 'candidates' set (no duplicates)
        std::set<std::pair<int, int>> candidates;
        for (const auto& [pos, chunk] : chunks) {
            for (int i = 0; i < 4; ++i) {
                auto [neighborPos, neighbor] = chunk->getNeighbor(i);
                if (neighbor == nullptr) {
                    candidates.insert(neighborPos);
                }
            }
        }

        // Out of all candidate chunks, find which ones are both within the
        // player's render distance and within the player's view frustum.
        // Load these chunks.
        float diff = CHUNK_WIDTH / 2.0f;
        auto [px, pz] = m_player->getPlayerChunk();
        sglm::frustum f = m_player->getFrustum();
        for (const auto& [x, z] : candidates) {
            int dist_sq = (x - px) * (x - px) + (z - pz) * (z - pz);
            if (dist_sq > Player::getLoadRadius() * Player::getLoadRadius())
                continue;
            bool contains = false;
            sglm::vec3 pos = { x *16 + diff, 0.0f, z * 16 + diff };
            for (int subchunk = 0; subchunk < NUM_SUBCHUNKS; ++subchunk) {
                pos.y = subchunk * SUBCHUNK_HEIGHT + diff;
                // +10 b/c some visible chunks on the edges of the frustum are not loading
                // TODO: find out what is causing this
                float r = SUB_CHUNK_RADIUS + 10.0f;
                if (m_player->getFrustum().contains(pos, r)) {
                    contains = true;
                    break;
                }
            }
            if (contains) {
                database::request_load(x, z);
            }
        }

        // Remove chunks that are beyond the player's render distance
        for (const auto& [pos, chunk] : chunks) {
            int x = pos.first, z = pos.second;
            int dist_sq = (px - x) * (px - x) + (pz - z) * (pz - z);
            if (dist_sq > Player::getLoadRadius() * Player::getLoadRadius()) {
                threadRemoveChunk(x, z, chunk);
            }
        }

        */

        // load chunks from the database
        database::Query q = database::get_load_result();
        while (q.type != database::QUERY_NONE) {
            assert(q.type == database::QUERY_LOAD);
            threadAddChunk(q.x, q.z, q.data);
            delete[] q.data;
            q = database::get_load_result();
        }

        // prevent busy waiting
        std::this_thread::sleep_for(50ms);

    }

    // thread is closing, unload all chunks
    while (!m_chunks.empty()) {
        auto itr = m_chunks.begin();
        int x = itr->first.first;
        int z = itr->first.second;
        Chunk* chunk = itr->second;
        threadRemoveChunk(x, z, chunk);
    }
    assert(m_chunks.empty());
}

void World::threadAddChunk(int x, int z, const void* data) {
    m_chunksMutex.lock();

    // if the chunk has already been loaded, don't do anything
    if (m_chunks.count({ x, z }) == 1) {
        m_chunksMutex.unlock();
        return;
    }

    // create the new chunk and add it to m_chunks
    // unlock while generating terrain for the chunk
    m_chunksMutex.unlock();
    Chunk* newChunk = new Chunk(x, z, data);
    m_chunksMutex.lock();
    m_chunks.emplace(std::make_pair(x, z), newChunk);

    // add neighbors to the new chunk.
    auto px = m_chunks.find({ x + 1, z });
    if (px != m_chunks.end()) {
        newChunk->addNeighbor(px->second, PLUS_X);
        px->second->addNeighbor(newChunk, MINUS_X);
    }
    auto mx = m_chunks.find({ x - 1, z });
    if (mx != m_chunks.end()) {
        newChunk->addNeighbor(mx->second, MINUS_X);
        mx->second->addNeighbor(newChunk, PLUS_X);
    }
    auto pz = m_chunks.find({ x, z + 1 });
    if (pz != m_chunks.end()) {
        newChunk->addNeighbor(pz->second, PLUS_Z);
        pz->second->addNeighbor(newChunk, MINUS_Z);
    }
    auto mz = m_chunks.find({ x, z - 1 });
    if (mz != m_chunks.end()) {
        newChunk->addNeighbor(mz->second, MINUS_Z);
        mz->second->addNeighbor(newChunk, PLUS_Z);
    }
    m_chunksMutex.unlock();
}

void World::threadRemoveChunk(int x, int z, Chunk* chunk) {
    m_chunksMutex.lock();
    assert(m_chunks.count({ x, z }) == 1);
    m_chunks.erase({ x, z });
    m_chunksMutex.unlock();
    if (chunk->wasUpdated()) {
        database::request_store(x, z, BLOCKS_PER_CHUNK, chunk->getBlockData());
    }
    delete chunk; // calls destructor, removes neighbors
}
