#include "Player.h"
#include "Face.h"
#include "Mesh.h"
#include "Shader.h"
#include "Constants.h"
#include <cassert>
#include <cmath>

inline constexpr sglm::vec3 WORLD_UP{ 0.0f, 1.0f, 0.0f };
inline constexpr float DEFAULT_YAW = -90.0f;
inline constexpr float DEFAULT_PITCH = 0.0f;
inline constexpr float DEFAULT_SPEED = 30.0f;
inline constexpr float DEFAULT_SENSITIVITY = 0.1f;
inline constexpr float DEFAULT_FOV = 60.0f;
inline constexpr float MIN_FOV = 5.0f;
inline constexpr float MAX_FOV = 90.0f;

int Player::load_radius = 10;
int Player::reach = 15;
std::pair<int, int> Player::chunks_rendered = { 0, 0 };

static inline float clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

int Player::getLoadRadius() {
    return Player::load_radius;
}

int Player::getUnloadRadius() {
    return Player::load_radius + 2;
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

Player::Player(sglm::vec3 position, float aspectRatio) : m_position{ position } {
    m_blockOutline = Mesh();
    m_aspectRatio = aspectRatio;
    m_yaw = DEFAULT_YAW;
    m_pitch = DEFAULT_PITCH;
    m_movementSpeed = DEFAULT_SPEED;
    m_mouseSensitivity = DEFAULT_SENSITIVITY;
    m_fov = DEFAULT_FOV;
    updateCamera();
    setProjectionMatrix();
}

const sglm::vec3& Player::getPosition() const {
    return m_position;
}

const sglm::vec3& Player::getDirection() const {
    return m_forward;
}

const sglm::mat4& Player::getViewMatrix() const {
    return m_viewMatrix;
}

const sglm::mat4& Player::getProjectionMatrix() const {
    return m_projectionMatrix;
}

const sglm::frustum& Player::getFrustum() const {
    return m_frustum;
}

void Player::setViewMatrix() {
    m_viewMatrix = sglm::look_at(m_position, m_position + m_forward, m_up);
    m_frustum.create(m_viewMatrix, m_projectionMatrix);
}

void Player::setProjectionMatrix() {
    m_projectionMatrix = sglm::perspective(sglm::radians(m_fov), m_aspectRatio, NEAR_PLANE, FAR_PLANE);
    m_frustum.create(m_viewMatrix, m_projectionMatrix);
}

void Player::updateCamera() {
    // calculate the forward vector using yaw, pitch, and WORLD_UP
    m_forward.x = std::cos(sglm::radians(m_yaw)) * std::cos(sglm::radians(m_pitch));
    m_forward.y = std::sin(sglm::radians(m_pitch));
    m_forward.z = std::sin(sglm::radians(m_yaw)) * std::cos(sglm::radians(m_pitch));
    m_forward = sglm::normalize(m_forward);

    // calculate the right and up vectors using the forward vector and WORLD_UP
    m_right = sglm::normalize(sglm::cross(m_forward, WORLD_UP));
    m_up = sglm::normalize(sglm::cross(m_right, m_forward));

    setViewMatrix();
}

void Player::look(float mouseX, float mouseY) {
    // only initialized on first function call
    static float lastMouseX = mouseX;
    static float lastMouseY = mouseY;

    // calculate new yaw and pitch given the change in mouse position
    // pitch is subtracted because for the position of the mouse, +y is down
    m_yaw += (mouseX - lastMouseX) * m_mouseSensitivity;
    m_pitch -= (mouseY - lastMouseY) * m_mouseSensitivity;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    // make sure the screen doesn't get flipped when pitch is out of bounds
    m_pitch = clamp(m_pitch, -89.9f, 89.9f);

    // update the forward, right, and up vectors to match the new yaw and pitch values
    updateCamera();
}

void Player::move(Movement direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime;
    switch (direction) {
        case Movement::FORWARD:  m_position = m_position + m_forward * velocity; break;
        case Movement::BACKWARD: m_position = m_position - m_forward * velocity; break;
        case Movement::LEFT:     m_position = m_position - m_right * velocity; break;
        case Movement::RIGHT:    m_position = m_position + m_right * velocity; break;
    }
    setViewMatrix();
}

void Player::setAspectRatio(float aspectRatio) {
    m_aspectRatio = aspectRatio;
    setProjectionMatrix();
}

void Player::setZoom(float zoom) {
    m_fov = clamp(m_fov - zoom, MIN_FOV, MAX_FOV);
    setProjectionMatrix();
}

void Player::setViewRayIsect(const Face::Intersection* isect) {
    if (isect == nullptr) {
        m_viewRayIntersection.x = -1;
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
    int x = (int) m_position.x / CHUNK_WIDTH - (m_position.x < 0);
    int y = (int) m_position.z / CHUNK_WIDTH - (m_position.z < 0);
    return { x, y };
}
