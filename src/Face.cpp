#include "Face.h"
#include "Constants.h"
#include <sglm/sglm.h>
#include <cmath>
#include <cassert>

// the points must be given in counter-clockwise order
Face::Face(sglm::vec3& a, sglm::vec3& b, sglm::vec3& c, sglm::vec3& d)
    : A{ a }, B{ b }, C{ c }, D{ d } {
    normal = sglm::normalize(sglm::cross(B - A, C - A));
}

bool Face::intersects(const sglm::ray& r, Intersection& isect) const {
    
    // Determine if the point is out of reach
    if (sglm::magnitude(r.pos - A) > PLAYER_REACH
        && sglm::magnitude(r.pos - B) > PLAYER_REACH
        && sglm::magnitude(r.pos - C) > PLAYER_REACH
        && sglm::magnitude(r.pos - D) > PLAYER_REACH) {
        return false;
    }

    // Determine if the ray intersects the plane of the face
    float d = -sglm::dot(normal, A);
    isect.t = -(sglm::dot(normal, r.pos) + d) / (sglm::dot(normal, r.dir));
    if (isect.t < 0) {
        return false;
    }
    sglm::vec3 Q = r.pos + r.dir * isect.t;
    if (std::abs(sglm::dot(normal, Q) + d) > 1e-6) {
        return false;
    }

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

void Face::Intersection::setData() {
    // find the x, y, and z values of the block that has this face
    if (A.x == B.x && A.x == C.x) {
        x = (int) (A.z > B.z ? A.x - 1.0f : A.x);
        y = (int) A.y;
        z = (int) (A.z > B.z ? B.z : A.z);
    }
    if (A.y == B.y && A.y == C.y) {
        x = (int) (A.x < C.x ? A.x : C.x);
        y = (int) (A.x < C.x ? A.y - 1.0f : A.y);
        z = (int) (A.x < C.x ? C.z : A.z);
    }
    if (A.z == B.z && A.z == C.z) {
        x = (int) (A.x < B.x ? A.x : B.x);
        y = (int) A.y;
        z = (int) (A.x < B.x ? A.z - 1.0f : A.z);
    }
    x = (x < 0 ? x % CHUNK_WIDTH + CHUNK_WIDTH : x) % CHUNK_WIDTH;
    z = (z < 0 ? z % CHUNK_WIDTH + CHUNK_WIDTH : z) % CHUNK_WIDTH;

    data[0] = 0b0010'00001'00000000'00001'00000'00000; // right (+x)
    data[1] = 0b0010'00001'00000000'00000'00000'00000;
    data[2] = 0b0010'00001'00000001'00000'00000'00000;
    data[3] = 0b0010'00001'00000001'00000'00000'00000;
    data[4] = 0b0010'00001'00000001'00001'00000'00000;
    data[5] = 0b0010'00001'00000000'00001'00000'00000;
    
    data[6] = 0b0010'00000'00000000'00000'00000'00000; // left (-x)
    data[7] = 0b0010'00000'00000000'00001'00000'00000;
    data[8] = 0b0010'00000'00000001'00001'00000'00000;
    data[9] = 0b0010'00000'00000001'00001'00000'00000;
    data[10] = 0b0010'00000'00000001'00000'00000'00000;
    data[11] = 0b0010'00000'00000000'00000'00000'00000;
    
    data[12] = 0b0001'00000'00000000'00001'00000'00000; // front (+z)
    data[13] = 0b0001'00001'00000000'00001'00000'00000;
    data[14] = 0b0001'00001'00000001'00001'00000'00000;
    data[15] = 0b0001'00001'00000001'00001'00000'00000;
    data[16] = 0b0001'00000'00000001'00001'00000'00000;
    data[17] = 0b0001'00000'00000000'00001'00000'00000;
    
    data[18] = 0b0001'00001'00000000'00000'00000'00000; // back (-z)
    data[19] = 0b0001'00000'00000000'00000'00000'00000;
    data[20] = 0b0001'00000'00000001'00000'00000'00000;
    data[21] = 0b0001'00000'00000001'00000'00000'00000;
    data[22] = 0b0001'00001'00000001'00000'00000'00000;
    data[23] = 0b0001'00001'00000000'00000'00000'00000;
    
    data[24] = 0b0011'00000'00000001'00001'00000'00000; // top (+y)
    data[25] = 0b0011'00001'00000001'00001'00000'00000;
    data[26] = 0b0011'00001'00000001'00000'00000'00000;
    data[27] = 0b0011'00001'00000001'00000'00000'00000;
    data[28] = 0b0011'00000'00000001'00000'00000'00000;
    data[29] = 0b0011'00000'00000001'00001'00000'00000;
    
    data[30] = 0b0000'00000'00000000'00000'00000'00000; // bottom (-y)
    data[31] = 0b0000'00001'00000000'00000'00000'00000;
    data[32] = 0b0000'00001'00000000'00001'00000'00000;
    data[33] = 0b0000'00001'00000000'00001'00000'00000;
    data[34] = 0b0000'00000'00000000'00001'00000'00000;
    data[35] = 0b0000'00000'00000000'00000'00000'00000;

    for (int i = 0; i < 6; ++i) {
        data[i * 6 + 0] += (x << 23) + (y << 15) + (z << 10) + 0x1E0;
        data[i * 6 + 1] += (x << 23) + (y << 15) + (z << 10) + 0x200;
        data[i * 6 + 2] += (x << 23) + (y << 15) + (z << 10) + 0x201;
        data[i * 6 + 3] += (x << 23) + (y << 15) + (z << 10) + 0x201;
        data[i * 6 + 4] += (x << 23) + (y << 15) + (z << 10) + 0x1E1;
        data[i * 6 + 5] += (x << 23) + (y << 15) + (z << 10) + 0x1E0;
    }
}

bool Face::Intersection::operator==(const Face::Intersection& other) const {
    return x == other.x && y == other.y && z == other.z;
}