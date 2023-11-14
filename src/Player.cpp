#include "Player.h"
#include "Camera.h"
#include "Face.h"
#include "Mesh.h"
#include "Shader.h"
#include "Constants.h"
#include <cassert>

int Player::load_radius = 8;
int Player::reach = 15;

int Player::getLoadRadius() {
    return Player::load_radius;
}

int Player::getUnloadRadius() {
    return Player::load_radius + 3;
}

void Player::setLoadRadius(int radius) {
    assert(radius > 0);
    Player::load_radius = radius;
}

int Player::getReach() {
    return Player::reach;
}

void Player::setReach(int r) {
    assert(r > 0);
    Player::reach = r;
}

Player::Player(Camera camera) : m_camera{ camera } {
    m_blockOutline = Mesh();
}

void Player::look(float xdiff, float ydiff) {
    m_camera.processMouseMovement(xdiff, ydiff);
}

void Player::move(Movement direction, float deltaTime) {
    m_camera.processKeyboard(direction, deltaTime);
}

void Player::setAspectRatio(float fov) {
    m_camera.processNewAspectRatio(fov);
}

void Player::setZoom(float zoom) {
    m_camera.processMouseScroll(zoom);
}

const Camera& Player::getCamera() const {
    return m_camera;
}

void Player::setViewRayIsect(const Face::Intersection* isect) {
    if (isect == nullptr) {
        m_blockOutline.erase();
    } else if (*isect != m_viewRayIntersection) {
        m_viewRayIntersection = *isect;
        m_blockOutline.generate(BYTES_PER_BLOCK, isect->data, false);
    }
}

const Face::Intersection& Player::getViewRayIsect() const {
    return m_viewRayIntersection;
}

bool Player::hasViewRayIsect() const {
    return m_blockOutline.generated();
}

void Player::renderOutline(const Shader* shader) const {
    m_blockOutline.render(shader);
}

std::pair<int, int> Player::getPlayerChunk() const {
    sglm::vec3 pos = m_camera.getPosition();
    int x = (int) pos.x / CHUNK_WIDTH - (pos.x < 0);
    int y = (int) pos.z / CHUNK_WIDTH - (pos.z < 0);
    return { x, y };
}
