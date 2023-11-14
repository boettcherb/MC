#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include "Camera.h"
#include "Mesh.h"
#include "Face.h"
#include "Shader.h"
#include "Constants.h"

class Player {
    static int load_radius;
    static int reach;

    // Camera contains:
    // 1. x,y,z position of the player (will have to adjust for height of player)
    // 2. the direction the player is looking (the vec3 m_forward)
    // 3. the player's view frustum
    // 4. Some settings such as fov, mouse sensitivity, and movement speed
    Camera m_camera;
    
    // the x,z coordinates of the chunk the player is in.
    // maybe instead of storing this, create a function that returns a pair
    int chunkX, chunkZ;

    // store info about the block this player is looking at
    Mesh m_blockOutline;
    Face::Intersection m_viewRayIntersection;


public:
    static int getLoadRadius();
    static int getUnloadRadius();
    static void setLoadRadius(int radius);
    static int getReach();
    static void setReach(int reach);

    Player(Camera camera);
    void look(float xdiff, float ydiff);
    void move(Movement direction, float deltaTime);
    void renderOutline(const Shader* shader) const;
    void setAspectRatio(float fov);
    void setZoom(float zoom);
    const Camera& getCamera() const;
    const Face::Intersection& getViewRayIsect() const;
    void setViewRayIsect(const Face::Intersection* isect);
    bool hasViewRayIsect() const;
    std::pair<int, int> getPlayerChunk() const;

};

#endif
