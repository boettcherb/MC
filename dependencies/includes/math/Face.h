#ifndef FACE_H_INCLUDED
#define FACE_H_INCLUDED

#include "sglm.h"

class Ray {
    Ray(sglm::vec3 pos, sglm::vec3 dir);
    sglm::vec3 position;
    sglm::vec3 direction;

public:
    sglm::vec3 getPosition() const;
    sglm::vec3 getDirection() const;
};

class Face {
    sglm::vec3 A, B, C, D;
    sglm::vec3 normal;

public:
    Face(sglm::vec3 a, sglm::vec3 b, sglm::vec3 c, sglm::vec3 d);
    bool intersects(const Ray* r) const;
};

#endif