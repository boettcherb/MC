#ifndef CAMERA_H_INCLUDED
#define CAMERA_H_INCLUDED

#include <math/sglm.h>

class Camera {
    sglm::vec3 m_position;
    sglm::vec3 m_forward, m_right, m_up;                // the camera's local axes
    float m_yaw, m_pitch;                               // euler angles (in degrees)
    float m_movementSpeed, m_mouseSensitivity, m_zoom;  // camera options

public:

    enum CameraMovement {
        FORWARD, BACKWARD, LEFT, RIGHT,
    };

    Camera(const sglm::vec3& initialPosition = { 0.0f, 0.0f, 0.0f });

    sglm::mat4 getViewMatrix() const;
    sglm::vec3 getCameraPosition() const;
    float getZoom() const;
    void processKeyboard(Camera::CameraMovement direction, float deltaTime);
    void processMouseMovement(float mouseX, float mouseY);
    void processMouseScroll(float offsetY);

private:
    void updateCamera();
};

#endif
