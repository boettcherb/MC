#include "Face.h"
#include "Constants.h"
#include <sglm/sglm.h>
#include <cmath>

Ray::Ray(sglm::vec3 pos, sglm::vec3 dir) : position{ pos }, direction{ dir } {}

sglm::vec3 Ray::getPosition() const {
    return position;
}

sglm::vec3 Ray::getDirection() const {
    return direction;
}

// the points must be given in counter-clockwise order
Face::Face(sglm::vec3& a, sglm::vec3& b, sglm::vec3& c, sglm::vec3& d, const unsigned int* data)
    : A{ a }, B{ b }, C{ c }, D{ d }, t{ -1 } {
    normal = sglm::normalize(sglm::cross(B - A, C - A));
    // store the data for this face while updating the texture coordinates
    faceData[0] = (data[0] & 0xFFFFFC00) + 0x1E0;
    faceData[1] = (data[1] & 0xFFFFFC00) + 0x200;
    faceData[2] = (data[2] & 0xFFFFFC00) + 0x201;
    faceData[3] = (data[3] & 0xFFFFFC00) + 0x201;
    faceData[4] = (data[4] & 0xFFFFFC00) + 0x1E1;
    faceData[5] = (data[5] & 0xFFFFFC00) + 0x1E0;
}

float Face::getT() const {
    return t;
}

unsigned int* Face::getData() {
    return faceData;
}

bool Face::intersects(const Ray& r) {
    // Determine if the point is out of reach
    if (sglm::magnitude(r.getPosition() - A) > PLAYER_REACH
        && sglm::magnitude(r.getPosition() - B) > PLAYER_REACH
        && sglm::magnitude(r.getPosition() - C) > PLAYER_REACH
        && sglm::magnitude(r.getPosition() - D) > PLAYER_REACH) {
        return false;
    }

    // Determine if the ray intersects the plane of the face
    float d = -sglm::dot(normal, A);
    t = -(sglm::dot(normal, r.getPosition()) + d) / (sglm::dot(normal, r.getDirection()));
    if (t < 0) {
        return false;
    }
    sglm::vec3 Q = r.getPosition() + r.getDirection() * t;
    if (std::abs(sglm::dot(normal, Q) + d) > 1e-6) {
        return false;
    }

    // Determine if the ray intersects the face
    if (sglm::dot(sglm::cross(B - A, Q - A), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(C - B, Q - B), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(D - C, Q - C), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(A - D, Q - D), normal) < 0.0) return false;

    return true;
}