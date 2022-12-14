#include "Camera.h"
#include <sglm/sglm.h>
#include <cmath>

const sglm::vec3 WORLD_UP = { 0.0f, 1.0f, 0.0f };
const float DEFAULT_YAW = -90.0f;
const float DEFAULT_PITCH = 0.0f;
const float DEFAULT_SPEED = 60.0f;
const float DEFAULT_SENSITIVITY = 0.1f;
const float DEFAULT_ZOOM = 45.0f;

static inline float clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

Camera::Camera(const sglm::vec3& initialPosition) : m_position{ initialPosition } {
    m_yaw = DEFAULT_YAW;
    m_pitch = DEFAULT_PITCH;
    m_movementSpeed = DEFAULT_SPEED;
    m_mouseSensitivity = DEFAULT_SENSITIVITY;
    m_zoom = DEFAULT_ZOOM;
    updateCamera();
}

sglm::mat4 Camera::getViewMatrix() const {
    return sglm::look_at(m_position, m_position + m_forward, m_up);
}

sglm::vec3 Camera::getCameraPosition() const {
    return m_position;
}

float Camera::getZoom() const {
    return m_zoom;
}

void Camera::processKeyboard(Camera::CameraMovement direction, float deltaTime) {
    float velocity = m_movementSpeed * deltaTime;
    switch (direction) {
        // case FORWARD:  m_position += m_forward * velocity; break;
        // case BACKWARD: m_position -= m_forward * velocity; break;
        // case LEFT:     m_position -= m_right * velocity; break;
        // case RIGHT:    m_position += m_right * velocity; break;

        case FORWARD:  m_position = m_position + m_forward * velocity; break;
        case BACKWARD: m_position = m_position - m_forward * velocity; break;
        case LEFT:     m_position = m_position - m_right * velocity; break;
        case RIGHT:    m_position = m_position + m_right * velocity; break;
    }
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
    m_zoom -= offsetY;

    // limit field of view to 1-45 degrees
    m_zoom = clamp(m_zoom, 1.0f, 45.0f);
}

void Camera::updateCamera() {
    // calculate the forward vector using yaw, pitch, and WORLD_UP
    sglm::vec3 newForward;
    newForward.x = std::cos(sglm::radians(m_yaw)) * std::cos(sglm::radians(m_pitch));
    newForward.y = std::sin(sglm::radians(m_pitch));
    newForward.z = std::sin(sglm::radians(m_yaw)) * std::cos(sglm::radians(m_pitch));
    m_forward = sglm::normalize(newForward);

    // calculate the right and up vectors using the forward vector and WORLD_UP
    m_right = sglm::normalize(sglm::cross(m_forward, WORLD_UP));
    m_up = sglm::normalize(sglm::cross(m_right, m_forward));
}
