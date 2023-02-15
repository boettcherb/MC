// Simple OpenGL Math Library
// I created this library to avoid the massive bloat and complexity of glm

#include "sglm.h"
#include <cmath>
#include <cassert>

namespace sglm {

    float radians(float deg) {
        return deg * 3.14159265358979f / 180.0f;
    }

    float magnitude(const vec3& v) {
        return std::sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    vec3 normalize(const vec3& v) {
        float mag = magnitude(v);
        assert(mag != 0.0f);
        return { v.x / mag, v.y / mag, v.z / mag };
    }

    vec3 vec3::operator+(const vec3& v) const {
        return { x + v.x, y + v.y, z + v.z };
    }

    vec3 vec3::operator-(const vec3& v) const {
        return { x - v.x, y - v.y, z - v.z };
    }

    vec3 vec3::operator*(float s) const {
        return { x * s, y * s, z * s };
    }

    float dot(const vec3& v1, const vec3& v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    vec3 cross(const vec3& v1, const vec3& v2) {
        float x = v1.y * v2.z - v1.z * v2.y;
        float y = v1.z * v2.x - v1.x * v2.z;
        float z = v1.x * v2.y - v1.y * v2.x;
        return { x, y, z };
    }

    mat4 translate(const vec3& v) {
        return {
            1,   0,   0,   0,
            0,   1,   0,   0,
            0,   0,   1,   0,
            v.x, v.y, v.z, 1,
        };
    }

    mat4 perspective(float v_fov, float ar, float near, float far) {
        float a = 1.0f / std::tanf(v_fov / 2.0f), n = near, f = far;
        return {
            a / ar, 0,                     0,  0,
            0,      a,                     0,  0,
            0,      0,     (f + n) / (n - f), -1,
            0,      0, (2 * f * n) / (n - f),  0,
        };
    }

    mat4 look_at(const vec3& from, const vec3& to, const vec3& up) {
        vec3 z = normalize(to - from) * -1;
        vec3 x = normalize(cross(up, z));
        vec3 y = cross(z, x);
        return {
            x.x,            y.x,           z.x,          0,
            x.y,            y.y,           z.y,          0,
            x.z,            y.z,           z.z,          0,
            -dot(from, x), -dot(from, y), -dot(from, z), 1,
        };
    }

}
