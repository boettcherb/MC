#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include <glm/glm.hpp>

class Camera {
    glm::vec3 m_position;
    glm::vec3 m_forward, m_right, m_up;                 // the camera's local axes
    float m_yaw, m_pitch;                               // euler angles (in degrees)
    float m_movementSpeed, m_mouseSensitivity, m_zoom;  // camera options

public:

    enum CameraMovement {
        FORWARD, BACKWARD, LEFT, RIGHT,
    };

    Camera(const glm::vec3& initialPosition = glm::vec3(0.0f));

    glm::mat4 getViewMatrix() const;
    glm::vec3 getCameraPosition() const;
    float getZoom() const;
    void processKeyboard(Camera::CameraMovement direction, float deltaTime);
    void processMouseMovement(float mouseX, float mouseY);
    void processMouseScroll(float offsetY);

private:
    void updateCamera();
};

#endif