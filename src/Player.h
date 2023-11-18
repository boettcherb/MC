#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "Mesh.h"
#include "Face.h"
#include "Shader.h"
#include "Constants.h"
#include <sglm/sglm.h>

class Player {
    static int load_radius;
    static int reach;

    sglm::vec3 m_position;
    sglm::vec3 m_forward, m_right, m_up;
    sglm::mat4 m_viewMatrix;
    sglm::mat4 m_projectionMatrix;
    sglm::frustum m_frustum;
    float m_yaw, m_pitch;                             // euler angles (in degrees)
    float m_movementSpeed, m_mouseSensitivity, m_fov; // camera options
    float m_aspectRatio;
    
    // store info about the block this player is looking at
    Mesh m_blockOutline;
    Face::Intersection m_viewRayIntersection;

public:
    static std::pair<int, int> chunks_rendered;
    static int getLoadRadius();
    static int getUnloadRadius();
    static void setLoadRadius(int radius);
    static int getReach();
    static void setReach(int reach);

    Player(sglm::vec3 position, float aspectRatio);
    void look(float xdiff, float ydiff);
    void move(Movement direction, float deltaTime);
    void renderOutline(const Shader* shader) const;
    void setAspectRatio(float aspectRatio);
    void setFOV(float fov);
    float getFOV() const;
    const Face::Intersection& getViewRayIsect() const;
    void setViewRayIsect(const Face::Intersection* isect);
    bool hasViewRayIsect() const;
    std::pair<int, int> getPlayerChunk() const;
    const sglm::vec3& getPosition() const;
    const sglm::vec3& getDirection() const;
    const sglm::mat4& getViewMatrix() const;
    const sglm::mat4& getProjectionMatrix() const;
    const sglm::frustum& getFrustum() const;

private:
    void updateCamera();
    void setViewMatrix();
    void setProjectionMatrix();

};

#endif
