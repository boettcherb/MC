#ifndef FACE_H_INCLUDED
#define FACE_H_INCLUDED

#include <sglm/sglm.h>

class Face {
    sglm::vec3 A, B, C, D;
    sglm::vec3 normal;
    unsigned int faceData[6];
    float t;

public:
    Face(sglm::vec3& a, sglm::vec3& b, sglm::vec3& c, sglm::vec3& d, const unsigned int* data);
    bool intersects(const sglm::ray& r);
    unsigned int* getData();
    float getT() const;
};

#endif
