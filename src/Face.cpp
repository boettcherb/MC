#include "Face.h"
#include "Constants.h"
#include "Block.h"
#include <sglm/sglm.h>
#include <cstring>
#include <cmath>
#include <cassert>

// the points must be given in counter-clockwise order
Face::Face(sglm::vec3& a, sglm::vec3& b, sglm::vec3& c, sglm::vec3& d, sglm::vec3& blockPosition) :
A{ a }, B{ b }, C{ c }, D{ d } {
    normal = sglm::normalize(sglm::cross(B - A, C - A));
    bx = (int) blockPosition.x;
    by = (int) blockPosition.y;
    bz = (int) blockPosition.z;
}

bool Face::intersects(const sglm::ray& r, Intersection& isect) const {
    // Determine if the point is out of reach
    if (sglm::magnitude(r.pos - A) > r.length
        && sglm::magnitude(r.pos - B) > r.length
        && sglm::magnitude(r.pos - C) > r.length
        && sglm::magnitude(r.pos - D) > r.length) {
        return false;
    }

    // Determine if the ray intersects the plane of the face
    float d = -sglm::dot(normal, A);
    isect.t = -(sglm::dot(normal, r.pos) + d) / (sglm::dot(normal, r.dir));
    if (isect.t < 0)
        return false;
    sglm::vec3 Q = r.pos + r.dir * isect.t;
    if (std::abs(sglm::dot(normal, Q) + d) > 1e-6)
        return false;

    // Determine if the ray intersects the face
    if (sglm::dot(sglm::cross(B - A, Q - A), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(C - B, Q - B), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(D - C, Q - C), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(A - D, Q - D), normal) < 0.0) return false;

    isect.A = A;
    isect.B = B;
    isect.C = C;
    isect.D = D;
    isect.x = bx;
    isect.y = by;
    isect.z = bz;

    return true;
}

bool Face::Intersection::operator==(const Face::Intersection& other) const {
    return x == other.x && y == other.y && z == other.z;
}

void Face::Intersection::operator=(const Face::Intersection& other) {
    // copy everything except data
    std::memcpy(this, &other, offsetof(Intersection, data));
}
