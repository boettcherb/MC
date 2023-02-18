#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include "Constants.h"
#include <sglm/sglm.h>

class Camera {
    sglm::vec3 m_position;
    sglm::vec3 m_forward, m_right, m_up;                // the camera's local axes
    sglm::mat4 m_viewMatrix;
    sglm::mat4 m_projectionMatrix;
    sglm::frustum m_frustum;
    float m_yaw, m_pitch;                               // euler angles (in degrees)
    float m_movementSpeed, m_mouseSensitivity, m_fov;   // camera options
    float m_aspectRatio;

public:
    Camera(const sglm::vec3& initialPosition, float aspectRatio);

    sglm::vec3 getPosition() const;
    sglm::vec3 getDirection() const;
    sglm::mat4 getViewMatrix() const;
    sglm::mat4 getProjectionMatrix() const;
    sglm::frustum getFrustum() const;
    void processKeyboard(Movement direction, float deltaTime);
    void processMouseMovement(float mouseX, float mouseY);
    void processMouseScroll(float offsetY);
    void processNewAspectRatio(float aspectRatio);

private:
    void updateCamera();
    void setViewMatrix();
    void setProjectionMatrix();
};

#endif
