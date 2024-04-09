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
    uint v2 = a_data.y;
    uint v3 = a_data.z;

    float xPos = float((v1 >> 12u) & 0xFu);
    float yPos = float((v1 >> 4u) & 0xFu);
    float zPos = float(v1 & 0xFu);
    xPos += (float((v2 >> 6u) & 0x3Fu)  - 16.0) / 16.0;
    yPos += (float(v2 & 0x3Fu)          - 16.0) / 16.0;
    zPos += (float((v3 >> 10u) & 0x3Fu) - 16.0) / 16.0;

    gl_Position = u2_projection * u1_view * u0_model * vec4(xPos, yPos, zPos, 1.0);

    float xTex = float((v3 >> 5u) & 0x1Fu);
    float yTex = float(v3 & 0x1Fu);
    v_texCoords = vec2(xTex / 16.0, yTex / 16.0);

    v_light = light[(v2 >> 12u) & 0x3u];
}
