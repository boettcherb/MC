#ifndef FACE_H_INCLUDED
#define FACE_H_INCLUDED

#include "Constants.h"
#include <sglm/sglm.h>

class Face {
    sglm::vec3 A, B, C, D;
    sglm::vec3 normal;

public:
    struct Intersection {
        int x, y, z; // coordinates of block whose face has the intersection
        int cx, cz; // coordinates of chunk the block is in
        float t; // TODO: do I really need to store t?
        sglm::vec3 A, B, C, D; // 4 corner positions of face
        unsigned int data[UINTS_PER_BLOCK];
        
        void setData();
        bool operator==(const Intersection& other) const;
        void operator=(const Intersection& other);
    };

    Face(sglm::vec3& a, sglm::vec3& b, sglm::vec3& c, sglm::vec3& d);
    bool intersects(const sglm::ray& r, Intersection& isect) const;
};

#endif
