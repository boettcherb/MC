#include "Camera.h"
#include "Constants.h"
#include <sglm/sglm.h>
#include <cmath>

static inline float clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

Camera::Camera(const sglm::vec3& initialPosition, float aspectRatio) {
    m_position = initialPosition;
    m_aspectRatio = aspectRatio;
    m_yaw = DEFAULT_YAW;
    m_pitch = DEFAULT_PITCH;
    m_movementSpeed = DEFAULT_SPEED;
    m_mouseSensitivity = DEFAULT_SENSITIVITY;
    m_fov = DEFAULT_FOV;
    updateCamera();
    setProjectionMatrix();
}

sglm::vec3 Camera::getPosition() const {
    return m_position;
}

sglm::vec3 Camera::getDirection() const {
    return m_forward;
}

sglm::mat4 Camera::getViewMatrix() const {
    return m_viewMatrix;
}

sglm::mat4 Camera::getProjectionMatrix() const {
    return m_projectionMatrix;
}

sglm::frustum Camera::getFrustum() const {
    return m_frustum;
}

void Camera::setViewMatrix() {
    m_viewMatrix = sglm::look_at(m_position, m_position + m_forward, m_up);
    m_frustum.create(m_viewMatrix, m_projectionMatrix);
}

void Camera::setProjectionMatrix() {
    m_projectionMatrix = sglm::perspective(sglm::radians(m_fov), m_aspectRatio, NEAR_PLANE, FAR_PLANE);
    m_frustum.create(m_viewMatrix, m_projectionMatrix);
}

void Camera::updateCamera() {
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

void Camera::processKeyboard(Movement direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime;
    switch (direction) {
        case Movement::FORWARD:  m_position = m_position + m_forward * velocity; break;
        case Movement::BACKWARD: m_position = m_position - m_forward * velocity; break;
        case Movement::LEFT:     m_position = m_position - m_right * velocity; break;
        case Movement::RIGHT:    m_position = m_position + m_right * velocity; break;
    }
    setViewMatrix();
}

void Camera::processMouseMovement(float mouseX, float mouseY) {
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

void Camera::processMouseScroll(float offsetY) {
    m_fov = clamp(m_fov - offsetY, MIN_FOV, MAX_FOV);
    setProjectionMatrix();
}

void Camera::processNewAspectRatio(float aspectRatio) {
    m_aspectRatio = aspectRatio;
    setProjectionMatrix();
}
