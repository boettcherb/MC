#include "World.h"
#include "Constants.h"
#include "Chunk.h"
#include "Shader.h"
#include "Player.h"
#include "Face.h"
#include "BlockInfo.h"
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
    int spawnRadius = 2;
    for (int x = cx - spawnRadius; x <= cx + spawnRadius; ++x) {
        for (int z = cz - spawnRadius; z <= cz + spawnRadius; ++z) {
            database::request_load(x, z);
        }
    }
    m_shader = shader;
    m_player = player;
    m_chunkLoaderThreadShouldClose = false;
    m_chunkLoaderThread = std::thread(&World::LoadChunks, this);
}

World::~World() {
    m_chunkLoaderThreadShouldClose = true;
    m_chunkLoaderThread.join();
    assert(m_chunks.empty());
}

// called once every frame
// mineBlock: true if the player has pressed the left mouse button. If the
// player is looking at a block, it will be mined.
// Update a max of 5 chunks per frame. This will prevent lag spikes if there
// are suddenly 40+ chunks to load
void World::update(bool mineBlock) {
    checkViewRayCollisions();

    // mine block we are looking at
    if (mineBlock && m_player->hasViewRayIsect()) {
        const Face::Intersection& isect = m_player->getViewRayIsect();
        m_chunksMutex.lock();
        Chunk* chunk = m_chunks.find({ isect.cx, isect.cz })->second;
        m_chunksMutex.unlock();
        chunk->put(isect.x, isect.y + SUBCHUNK_HEIGHT * isect.cy, isect.z, Block::BlockType::AIR);
    }

    int numUpdated = 0;
    m_chunksMutex.lock();
    for (const auto& [_, chunk] : m_chunks) {
        numUpdated += chunk->update();
        if (numUpdated >= 5)
            break;
    }
    m_chunksMutex.unlock();
}

// determine if the player is looking at a block (if yes, we
// want to render a block outline around that block)
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
                    // bestI.cy is set in Chunk::intersects
                }
            }
        }
    }
    m_chunksMutex.unlock();
    if (foundIntersection) {
        // fill in the data field of the intersection with the block outline's vertex data
        // for simplicity, set the surrounding blocks to AIR so the entire blockoutline renders
        std::array<Block::BlockType, NUM_DIRECTIONS> surrounding;
        surrounding.fill(Block::BlockType::AIR);
        Block::getBlockData(Block::BlockType::OUTLINE, bestI.x, bestI.y,
                            bestI.z, bestI.data, surrounding);
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
        float y = (float) (isect.cy * SUBCHUNK_HEIGHT);
        float z = (float) (isect.cz * CHUNK_WIDTH);
        m_shader->addUniformMat4f("u0_model", sglm::translate({ x, y, z }));
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

void World::LoadChunks() {
    using namespace std::chrono_literals;
    while (!m_chunkLoaderThreadShouldClose) {

        // Find all unloaded chunks that are adjacent to loaded chunks. Add
        // these chunks to the 'candidates' set (no duplicates)
        std::set<std::pair<int, int>> candidates;
        for (const auto& [pos, chunk] : m_chunks) {
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
            sglm::vec3 pos = { x * CHUNK_WIDTH + diff, 0.0f, z * CHUNK_WIDTH + diff };
            for (int subchunk = 0; subchunk < NUM_SUBCHUNKS; ++subchunk) {
                pos.y = subchunk * SUBCHUNK_HEIGHT + diff;
                if (m_player->getFrustum().contains(pos, SUB_CHUNK_RADIUS)) {
                    contains = true;
                    break;
                }
            }
            if (contains) {
                database::request_load(x, z);
            }
        }

        // create a copy of the chunks so I can loop through through all the
        // chunks and add/remove chunks at the same time.
        std::vector<std::pair<std::pair<int, int>, Chunk*>> chunks;
        chunks.reserve(m_chunks.size());
        for (const auto& [pos, chunk] : m_chunks) {
            chunks.push_back({ pos, chunk });
        }
        // Remove chunks that are beyond the player's render distance
        for (const auto& [pos, chunk] : chunks) {
            int x = pos.first, z = pos.second;
            int dist_sq = (px - x) * (px - x) + (pz - z) * (pz - z);
            if (dist_sq > Player::getUnloadRadius() * Player::getUnloadRadius()) {
                removeChunk(x, z, chunk);
            }
        }

        // load chunks from the database
        database::Query q = database::get_load_result();
        while (q.type != database::QUERY_NONE) {
            assert(q.type == database::QUERY_LOAD);
            addChunk(q.x, q.z, q.data);
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
        removeChunk(x, z, chunk);
    }
    assert(m_chunks.empty());
}

void World::addChunk(int x, int z, const void* data) {
    // if the chunk has already been loaded, don't do anything
    if (m_chunks.count({ x, z }) == 1) {
        return;
    }

    // create the new chunk and add it to m_chunks
    // unlock while generating terrain for the chunk
    const Block::BlockType* block_data = reinterpret_cast<const Block::BlockType*>(data);
    Chunk* newChunk = new Chunk(x, z, block_data);
    m_chunksMutex.lock();
    m_chunks.emplace(std::make_pair(x, z), newChunk);
    m_chunksMutex.unlock();

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
}

void World::removeChunk(int x, int z, Chunk* chunk) {
    assert(m_chunks.count({ x, z }) == 1);
    m_chunksMutex.lock();
    m_chunks.erase({ x, z });
    m_chunksMutex.unlock();
    if (chunk->wasUpdated()) {
        database::request_store(x, z, BLOCKS_PER_CHUNK, chunk->getBlockData());
    }
    delete chunk; // calls destructor, removes neighbors
}
