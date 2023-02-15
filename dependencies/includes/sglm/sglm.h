// Simple openGL Math Library
// I create this to avoid the massive bloat and complexity of glm

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

}

#endif
