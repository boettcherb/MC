#include "World.h"
#include "Constants.h"
#include "Chunk.h"
#include "Shader.h"
#include "Player.h"
#include "Face.h"
#include "Block.h"
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

World::World(Shader* shader, Player* player) : m_shader{ shader },
m_player{ player }, m_chunkLoaderThreadShouldClose{ false } {
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
    m_player->chunks_rendered = { rendered, total };
}

static inline bool within_distance(int px, int pz, int cx, int cz, int dist) {
    int dist_sq = (px - cx) * (px - cx) + (pz - cz) * (pz - cz);
    return dist_sq <= dist * dist;
}

static void checkIfUpdated(Chunk* chunk, const std::pair<int, int>& pos) {
    if (chunk->wasUpdated()) {
        auto& [cx, cz] = pos;
        database::request_store(cx, cz, BLOCKS_PER_CHUNK, chunk->getBlockData());
        chunk->updateHandled();
    }
}

void World::LoadChunks() {
    using namespace std::chrono_literals;
    while (!m_chunkLoaderThreadShouldClose) {
        bool updateMade = false;
        auto [px, pz] = m_player->getPlayerChunk();

        // Make sure that all chunks within Player::getLoadRadius() of the player are loaded
        for (int x = px - Player::getLoadRadius(); x <= px + Player::getLoadRadius(); ++x) {
            for (int z = pz - Player::getLoadRadius(); z <= pz + Player::getLoadRadius(); ++z) {
                if (within_distance(px, pz, x, z, Player::getLoadRadius())) {
                    if (m_chunks.find({ x, z }) == m_chunks.end()) {
                        addChunk(x, z);
                        updateMade = true;
                    }
                }
            }
        }
        
        // Loop through every chunk:
        // If a chunk is beyond the player's load radius, unload it
        // If a chunk has Status::EMPTY, generate structures for it
        // If a chunk is within the player's render distance and view frustum and does not
        //     have Status::LOADING or higher, try loading it from the database.
        // If a chunk is outside the player's un-render distance and has Status::FULL
        //     or Status::TERRAIN, un-render it (delete its mesh and block data)
        // 
        std::vector<std::pair<std::pair<int, int>, Chunk*>> need_to_remove;
        need_to_remove.reserve(64);
        for (const auto& [pos, chunk] : m_chunks) {
            const auto& [cx, cz] = pos;
            if (!within_distance(px, pz, cx, cz, Player::getLoadRadius())) {
                if (chunk->getStatus() <= Chunk::Status::STRUCTURES) {
                    need_to_remove.push_back({ pos, chunk });
                } else {
                    checkIfUpdated(chunk, pos);
                    chunk->setToDelete();
                }
                updateMade = true;
            }
            else if (chunk->getStatus() == Chunk::Status::EMPTY) {
                chunk->generateStructures();
                assert(chunk->getStatus() == Chunk::Status::STRUCTURES);
                updateMade = true;
            }
            else if (chunk->getStatus() < Chunk::Status::LOADING && within_distance(px, pz, cx, cz, Player::getRenderDist())) {
                bool contains = false;
                float diff = CHUNK_WIDTH / 2.0f;
                sglm::vec3 sc_center = { cx * CHUNK_WIDTH + diff, 0.0f, cz * CHUNK_WIDTH + diff };
                for (int subchunk = 0; subchunk < NUM_SUBCHUNKS; ++subchunk) {
                    sc_center.y = subchunk * SUBCHUNK_HEIGHT + diff;
                    if (m_player->getFrustum().contains(sc_center, SUB_CHUNK_RADIUS * 2)) {
                        contains = true;
                        break;
                    }
                }
                if (contains) {
                    database::request_load(cx, cz);
                    assert(m_chunks.find({ cx, cz }) != m_chunks.end());
                    (*m_chunks.find({ cx, cz })).second->setLoading();
                    updateMade = true;
                }
            }
            else if (chunk->getStatus() >= Chunk::Status::TERRAIN && !within_distance(px, pz, cx, cz, Player::getUnRenderDist())) {
                checkIfUpdated(chunk, pos);
                chunk->setToDelete();
                updateMade = true;
            }
        }
        for (const auto& [pos, chunk] : need_to_remove) {
            auto& [x, z] = pos;
            removeChunk(x, z, chunk);
        }

        // load chunks from the database
        database::Query q = database::get_load_result();
        while (q.type != database::QUERY_NONE) {
            assert(q.type == database::QUERY_LOAD);
            assert(m_chunks.find({ q.x, q.z }) != m_chunks.end());
            const auto& [pos, chunk] = *m_chunks.find({ q.x, q.z });
            assert(chunk->getStatus() == Chunk::Status::LOADING);
            if (q.data != nullptr) {
                chunk->addBlockData(reinterpret_cast<const Block::BlockType*>(q.data));
                delete[] q.data;
            } else {
                Block::BlockType* data = new Block::BlockType[BLOCKS_PER_CHUNK];
                chunk->generateTerrain(data, 1337);
                chunk->addBlockData(data);
                delete[] data;
            }
            updateMade = true;
            q = database::get_load_result();
        }

        if (!updateMade) {
            std::this_thread::sleep_for(50ms);
        }
    }

    // thread is closing, unload all chunks
    for (const auto& [pos, chunk] : m_chunks) {
        if (chunk->getStatus() >= Chunk::Status::TERRAIN) {
            checkIfUpdated(chunk, pos);
            chunk->deleteBlockData();
        }
    }
    while (!m_chunks.empty()) {
        const auto& [pos, chunk] = *m_chunks.begin();
        removeChunk(pos.first, pos.second, chunk);
    }
    assert(m_chunks.empty());
}

void World::addChunk(int x, int z) {
    // if the chunk has already been loaded, don't do anything
    assert(m_chunks.find({ x, z }) == m_chunks.end());

    // create the new chunk and add it to m_chunks
    Chunk* newChunk = new Chunk(x, z);
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
    assert(m_chunks.find({ x, z }) != m_chunks.end());
    assert(chunk->getStatus() < Chunk::Status::TERRAIN);
    assert(!chunk->wasUpdated());
    m_chunksMutex.lock();
    m_chunks.erase({ x, z });
    m_chunksMutex.unlock();
    delete chunk; // calls destructor, removes neighbors
}
