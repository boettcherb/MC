#include "Face.h"
#include "Constants.h"
#include <sglm/sglm.h>
#include <cmath>






// #include <iostream>








// the points must be given in counter-clockwise order
Face::Face(sglm::vec3& a, sglm::vec3& b, sglm::vec3& c, sglm::vec3& d, const unsigned int* data)
    : A{ a }, B{ b }, C{ c }, D{ d }, t{ -1 } {
    normal = sglm::normalize(sglm::cross(B - A, C - A));
    // store the data for this face while updating the texture coordinates
    // faceData[0] = (data[0] & 0xFFFFFC00) + 0x1E0; // bottom left
    // faceData[1] = (data[1] & 0xFFFFFC00) + 0x200; // bottom right
    // faceData[2] = (data[2] & 0xFFFFFC00) + 0x201; // top right
    // faceData[3] = (data[3] & 0xFFFFFC00) + 0x201; // top right
    // faceData[4] = (data[4] & 0xFFFFFC00) + 0x1E1; // top left
    // faceData[5] = (data[5] & 0xFFFFFC00) + 0x1E0; // bottom left

    faceData[0] = (data[0] & 0xFFFFFC00) + 0x201;
    faceData[1] = (data[1] & 0xFFFFFC00) + 0x1E1;
    faceData[2] = (data[2] & 0xFFFFFC00) + 0x1E0;
    faceData[3] = (data[3] & 0xFFFFFC00) + 0x1E0;
    faceData[4] = (data[4] & 0xFFFFFC00) + 0x200;
    faceData[5] = (data[5] & 0xFFFFFC00) + 0x201;
}

float Face::getT() const {
    return t;
}

unsigned int* Face::getData() {
    return faceData;
}

bool Face::intersects(const sglm::ray& r) {
    sglm::vec3 r_c = { std::fmodf(r.pos.x, 16), r.pos.y, std::fmodf(r.pos.z, 16) };
    if (r_c.x < 0) r_c.x += 16.0f;
    if (r_c.z < 0) r_c.z += 16.0f;
    // std::cout << "    Face:";
    // std::cout << " (" << A.x << ", " << A.y << ", " << A.z << ")";
    // std::cout << " (" << B.x << ", " << B.y << ", " << B.z << ")"; 
    // std::cout << " (" << C.x << ", " << C.y << ", " << C.z << ")";
    // std::cout << " (" << D.x << ", " << D.y << ", " << D.z << ")" << std::endl;
    // std::cout << "    Normal: " << normal.x << ", " << normal.y << ", " << normal.z << ")\n";
    // std::cout << "r_c: " << r_c.x << ' ' << r_c.y << ' ' << r_c.z << '\n';
    // Determine if the point is out of reach
    if (sglm::magnitude(r_c - A) > PLAYER_REACH
        && sglm::magnitude(r_c - B) > PLAYER_REACH
        && sglm::magnitude(r_c - C) > PLAYER_REACH
        && sglm::magnitude(r_c - D) > PLAYER_REACH) {
        // std::cout << "      player out of reach" << std::endl;
        return false;
    }

    // Face: (0, 72, 1) (1, 72, 1) (1, 72, 0) (0, 72, 0)
    // (16.5004, 74.6778, 16.4907)

    // Determine if the ray intersects the plane of the face
    float d = -sglm::dot(normal, A);
    t = -(sglm::dot(normal, r_c) + d) / (sglm::dot(normal, r.dir));
    if (t < 0) {
        // std::cout << "      t < 0\n";
        return false;
    }
    sglm::vec3 Q = r_c + r.dir * t;
    if (std::abs(sglm::dot(normal, Q) + d) > 1e-6) {
        // std::cout << "      ray does not intersect plane\n";
        return false;
    }

    // Determine if the ray intersects the face
    if (sglm::dot(sglm::cross(B - A, Q - A), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(C - B, Q - B), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(D - C, Q - C), normal) < 0.0) return false;
    if (sglm::dot(sglm::cross(A - D, Q - D), normal) < 0.0) return false;
    // std::cout << "FOUND A COLLISION\n";
    return true;
}
