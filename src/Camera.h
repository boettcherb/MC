#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "Constants.h"
#include <sglm/sglm.h>

class Camera {
    sglm::vec3 m_position;
    sglm::vec3 m_forward, m_right, m_up;                // the camera's local axes
    sglm::mat4 m_viewMatrix;
    float m_yaw, m_pitch;                               // euler angles (in degrees)
    float m_movementSpeed, m_mouseSensitivity, m_fov;   // camera options

public:
    Camera(const sglm::vec3& initialPosition = { 0.0f, 0.0f, 0.0f });

    sglm::mat4 getViewMatrix() const;
    sglm::vec3 getPosition() const;
    sglm::vec3 getDirection() const;
    float getFOV() const;
    void processKeyboard(Movement direction, float deltaTime);
    void processMouseMovement(float mouseX, float mouseY);
    void processMouseScroll(float offsetY);

private:
    void updateCamera();
    void setViewMatrix();
};

#endif
