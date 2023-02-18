// Simple openGL Math Library
// 
// This library is one header only (stb style).
// In one .cpp file, define SGLM_IMPLEMENTATION before the include statement:
//      #define SGLM_IMPLEMENTATION
//      #include <sglm.h>
//

#ifndef SGLM_H_INCLUDED
#define SGLM_H_INCLUDED

namespace sglm {

    float radians(float deg);

    struct vec3 {
        float x, y, z;
        vec3 operator+(const vec3& v) const;
        vec3 operator-(const vec3& v) const;
        vec3 operator*(float s) const;
    };

    float magnitude(const vec3& v);
    vec3 normalize(const vec3& v);
    float dot(const vec3& v1, const vec3& v2);
    vec3 cross(const vec3& v1, const vec3& v2);

    struct mat4 {
        float m[16];
    };

    mat4 translate(const vec3& v);
    mat4 perspective(float v_fov, float ar, float near, float far);
    mat4 look_at(const vec3& from, const vec3& to, const vec3& up);

    struct ray {
        vec3 pos;
        vec3 dir;
    };

    struct plane {
        plane() = default;
        plane(const vec3& norm, const vec3& p);
        vec3 normal;
        float d;
    };

    struct frustum {
        plane planes[6];
        frustum(const vec3& position, const vec3& forward, float fov,
                float screen_ratio, float near, float far);
        bool contains(const vec3& pos, float radius) const;
    };
}

#endif // SGLM_H_INCLUDED

#ifdef SGLM_IMPLEMENTATION

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
            x.x,           y.x,           z.x,           0,
            x.y,           y.y,           z.y,           0,
            x.z,           y.z,           z.z,           0,
            -dot(from, x), -dot(from, y), -dot(from, z), 1,
        };
    }

    plane::plane(const vec3& norm, const vec3& p) {
        normal = normalize(norm);
        d = -dot(normal, p);
    }

    // forward must be normalized
    frustum::frustum(const vec3& position, const vec3& forward, float v_fov,
                     float screen_ratio, float near, float far) {
        assert(std::abs(magnitude(forward) - 1) < 1e-6);
        vec3 right = normalize(cross(forward, { 0.0f, 1.0f, 0.0f }));
        vec3 up = normalize(cross(right, forward));
        vec3 cam_to_far = forward * far;
        float half_v = far * std::tanf(v_fov * 0.5f);
        float half_h = half_v * screen_ratio;
        planes[0] = plane(cross(cam_to_far - right * half_h, up), position); // LEFT
        planes[1] = plane(cross(up, cam_to_far + right * half_h), position); // RIGHT
        planes[2] = plane(cross(cam_to_far + up * half_v, right), position); // TOP
        planes[3] = plane(cross(right, cam_to_far - up * half_v), position); // BOTTOM
        planes[4] = plane(forward, position + forward * near);               // NEAR
        planes[5] = plane(forward * -1, position + cam_to_far);              // FAR
    }

    bool frustum::contains(const vec3& pos, float radius) const {
        for (const plane& p : planes) {
            if (dot(p.normal, pos) + p.d + radius <= 0) {
                return false;
            }
        }
        return true;
    }

}

#endif // SGLM_IMPLEMENTATION
