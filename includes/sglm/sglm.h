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
        vec3 operator/(float s) const;
    };

    float magnitude(const vec3& v);
    vec3 normalize(const vec3& v);
    float dot(const vec3& v1, const vec3& v2);
    vec3 cross(const vec3& v1, const vec3& v2);

    struct mat4 {
        float m[16];
        mat4 operator*(const mat4& mat) const;
    };

    mat4 translate(const vec3& v);
    mat4 perspective(float v_fov, float ar, float near, float far);
    mat4 look_at(const vec3& from, const vec3& to, const vec3& up);

    struct ray {
        vec3 pos;
        vec3 dir;
    };

    struct plane {
        vec3 normal;
        float d;
        void normalize_plane();
    };

    struct frustum {
        plane planes[6];
        void create(const mat4& view, const mat4& projection);
        bool contains(const vec3& pos, float radius) const;
    };

    void print_vec3(const vec3& vec);
    void print_mat4(const mat4& mat);
    void print_plane(const plane& p);
}

#endif // SGLM_H_INCLUDED

#ifdef SGLM_IMPLEMENTATION

#include <cmath>
#include <cassert>
#include <iostream>

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
        return v / mag;
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

    vec3 vec3::operator/(float s) const {
        return { x / s, y / s, z / s };
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

    mat4 mat4::operator*(const mat4& mat) const {
        return {
            m[0] * mat.m[0] + m[1] * mat.m[4] + m[2] * mat.m[8] + m[3] * mat.m[12],
            m[0] * mat.m[1] + m[1] * mat.m[5] + m[2] * mat.m[9] + m[3] * mat.m[13],
            m[0] * mat.m[2] + m[1] * mat.m[6] + m[2] * mat.m[10] + m[3] * mat.m[14],
            m[0] * mat.m[3] + m[1] * mat.m[7] + m[2] * mat.m[11] + m[3] * mat.m[15],
            m[4] * mat.m[0] + m[5] * mat.m[4] + m[6] * mat.m[8] + m[7] * mat.m[12],
            m[4] * mat.m[1] + m[5] * mat.m[5] + m[6] * mat.m[9] + m[7] * mat.m[13],
            m[4] * mat.m[2] + m[5] * mat.m[6] + m[6] * mat.m[10] + m[7] * mat.m[14],
            m[4] * mat.m[3] + m[5] * mat.m[7] + m[6] * mat.m[11] + m[7] * mat.m[15],
            m[8] * mat.m[0] + m[9] * mat.m[4] + m[10] * mat.m[8] + m[11] * mat.m[12],
            m[8] * mat.m[1] + m[9] * mat.m[5] + m[10] * mat.m[9] + m[11] * mat.m[13],
            m[8] * mat.m[2] + m[9] * mat.m[6] + m[10] * mat.m[10] + m[11] * mat.m[14],
            m[8] * mat.m[3] + m[9] * mat.m[7] + m[10] * mat.m[11] + m[11] * mat.m[15],
            m[12] * mat.m[0] + m[13] * mat.m[4] + m[14] * mat.m[8] + m[15] * mat.m[12],
            m[12] * mat.m[1] + m[13] * mat.m[5] + m[14] * mat.m[9] + m[15] * mat.m[13],
            m[12] * mat.m[2] + m[13] * mat.m[6] + m[14] * mat.m[10] + m[15] * mat.m[14],
            m[12] * mat.m[3] + m[13] * mat.m[7] + m[14] * mat.m[11] + m[15] * mat.m[15],
        };
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

    void plane::normalize_plane() {
        float mag = magnitude(normal);
        normal = normal / mag;
        d /= mag;
    }

    // Gribb-Hartmann method for extracting the frustum planes
    // out of the view and projection matrices
    void frustum::create(const mat4& view, const mat4& projection) {
        mat4 mat = view * projection;
        // LEFT
        planes[0].normal.x = mat.m[3] + mat.m[0];
        planes[0].normal.y = mat.m[7] + mat.m[4];
        planes[0].normal.z = mat.m[11] + mat.m[8];
        planes[0].d = mat.m[15] + mat.m[12];
        planes[0].normalize_plane();
        // RIGHT
        planes[1].normal.x = mat.m[3] - mat.m[0];
        planes[1].normal.y = mat.m[7] - mat.m[4];
        planes[1].normal.z = mat.m[11] - mat.m[8];
        planes[1].d = mat.m[15] - mat.m[12];
        planes[1].normalize_plane();
        // TOP
        planes[2].normal.x = mat.m[3] - mat.m[1];
        planes[2].normal.y = mat.m[7] - mat.m[5];
        planes[2].normal.z = mat.m[11] - mat.m[9];
        planes[2].d = mat.m[15] - mat.m[13];
        planes[2].normalize_plane();
        // BOTTOM
        planes[3].normal.x = mat.m[3] + mat.m[1];
        planes[3].normal.y = mat.m[7] + mat.m[5];
        planes[3].normal.z = mat.m[11] + mat.m[9];
        planes[3].d = mat.m[15] + mat.m[13];
        planes[3].normalize_plane();
        // NEAR
        planes[4].normal.x = mat.m[3] + mat.m[2];
        planes[4].normal.y = mat.m[7] + mat.m[6];
        planes[4].normal.z = mat.m[11] + mat.m[10];
        planes[4].d = mat.m[15] + mat.m[14];
        planes[4].normalize_plane();
        // FAR
        planes[5].normal.x = mat.m[3] - mat.m[2];
        planes[5].normal.y = mat.m[7] - mat.m[6];
        planes[5].normal.z = mat.m[11] - mat.m[10];
        planes[5].d = mat.m[15] - mat.m[14];
        planes[5].normalize_plane();
    }

    bool frustum::contains(const vec3& pos, float radius) const {
        for (const plane& p : planes) {
            if (dot(p.normal, pos) + p.d + radius <= 0) {
                return false;
            }
        }
        return true;
    }

    void print_vec3(const vec3& vec) {
        printf("[ %8.3f %8.3f %8.3f ]\n", vec.x, vec.y, vec.z);
    }

    void print_mat4(const mat4& mat) {
        for (int i = 0; i < 4; ++i) {
            printf("[ ");
            for (int j = 0; j < 4; ++j) {
                printf("%8.3f ", mat.m[i * 4 + j]);
            }
            printf("]\n");
        }
    }

    void print_plane(const plane& p) {
        print_vec3(p.normal);
        printf("d: %f\n", p.d);
    }

}

#endif // SGLM_IMPLEMENTATION
