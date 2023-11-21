#include "Face.h"
#include "Constants.h"
#include "BlockInfo.h"
#include <sglm/sglm.h>
#include <cstring>
#include <cmath>
#include <cassert>

// the points must be given in counter-clockwise order
Face::Face(sglm::vec3& a, sglm::vec3& b, sglm::vec3& c, sglm::vec3& d)
    : A{ a }, B{ b }, C{ c }, D{ d } {
    normal = sglm::normalize(sglm::cross(B - A, C - A));
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
    return true;
}

// Find the x, y, and z values of the block that has this face
// and fill in the data member of the intersection.
void Face::Intersection::setData() {
    if (A.x == B.x && A.x == C.x) {                // +x or -x
        x = (int) (A.z < B.z ? A.x : A.x - 1.0f);
        y = (int) A.y;
        z = (int) (A.z < B.z ? A.z : B.z);
    }
    else if (A.y == B.y && A.y == C.y) {           // +y or -y
        x = (int) A.x;
        y = (int) (A.z < C.z ? A.y : A.y - 1.0f);
        z = (int) (A.z < C.z ? A.z : C.z);
    }
    else {                                         // +z or -z
        x = (int) (A.x < B.x ? A.x : B.x);
        y = (int) A.y;
        z = (int) (A.x < B.x ? A.z - 1.0f : A.z);
    }
    x = (x < 0 ? x % CHUNK_WIDTH + CHUNK_WIDTH : x) % CHUNK_WIDTH;
    z = (z < 0 ? z % CHUNK_WIDTH + CHUNK_WIDTH : z) % CHUNK_WIDTH;

    Block::getBlockData(Block::BlockType::OUTLINE, x, y, z, data);
}

bool Face::Intersection::operator==(const Face::Intersection& other) const {
    return x == other.x && y == other.y && z == other.z;
}

void Face::Intersection::operator=(const Face::Intersection& other) {
    // copy everything except data
    std::memcpy(this, &other, offsetof(Intersection, data));
}
