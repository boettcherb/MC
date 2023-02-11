#include "Face.h"
#include "sglm.h"
#include <cmath>

// the player can reach up to 5 blocks away
static constexpr int MAX_PLAYER_REACH = 5;

Ray::Ray(sglm::vec3 pos, sglm::vec3 dir) : position{ pos }, direction{ dir } {}

sglm::vec3 Ray::getPosition() const {
    return position;
}

sglm::vec3 Ray::getDirection() const {
    return direction;
}

// the points must be given in counter-clockwise order
Face::Face(sglm::vec3 a, sglm::vec3 b, sglm::vec3 c, sglm::vec3 d) : A{ a }, B{ b }, C{ c }, D{ d } {
    normal = sglm::normalize(sglm::cross(B - A, C - A));
}

bool Face::intersects(const Ray* r) const {
    // Determine if the point is out of reach
    if (sglm::magnitude(r->getPosition() - A) > MAX_PLAYER_REACH
     && sglm::magnitude(r->getPosition() - B) > MAX_PLAYER_REACH
     && sglm::magnitude(r->getPosition() - C) > MAX_PLAYER_REACH
     && sglm::magnitude(r->getPosition() - D) > MAX_PLAYER_REACH) {
        return false;
    }

    // Determine if the ray intersects the plane of the face
    float constant_d = -sglm::dot(normal, A);
    float t = -(sglm::dot(normal, r->getPosition()) + constant_d) / (sglm::dot(normal, r->getDirection()));
    if (t < 0) {
        return false;
    }
    sglm::vec3 Q = r->getPosition() + r->getDirection() * t;
    if (std::abs(sglm::dot(normal, Q) + constant_d) > 1e-6) {
        return false;
    }

    // Determine if the ray intersects the face
    if (sglm::dot(sglm::cross(B - A, Q - A), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(C - B, Q - B), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(D - C, Q - C), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(A - D, Q - D), normal) < 0.0) return false;

    return true;
}
