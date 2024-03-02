#version 330 core

layout(location = 0) in uvec3 a_data;

out vec2 v_texCoords;
out float v_light;

uniform mat4 u0_model;
uniform mat4 u1_view;
uniform mat4 u2_projection;

const float light[4] = { 0.4, 0.6, 0.8, 1.0 };

void main() {

    uint v1 = a_data.x;
    uint v3 = a_data.z;

    float xPos = float((v1 >> 23u) & 0x1Fu);
    float yPos = float((v1 >> 15u) & 0xFFu);
    float zPos = float((v3 >> 10u) & 0x1Fu);

    gl_Position = u2_projection * u1_view * u0_model * vec4(xPos, yPos, zPos, 1.0);

    float xTex = float((v3 >> 5u) & 0x1Fu);
    float yTex = float(v3 & 0x1Fu);
    v_texCoords = vec2(xTex / 16.0, yTex / 16.0);

    v_light = light[(v1 >> 28u) & 0x3u];
}
