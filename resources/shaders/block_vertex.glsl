#version 330 core

layout(location = 0) in uint a_data;

out vec2 v_texCoords;
out float v_light;

uniform mat4 u0_model;
uniform mat4 u1_view;
uniform mat4 u2_projection;

const float light[4] = { 0.4, 0.6, 0.8, 1.0 };

void main() {
    float xPos = float((a_data >> 23u) & 0x1Fu);
    float yPos = float((a_data >> 15u) & 0xFFu);
    float zPos = float((a_data >> 10u) & 0x1Fu);

    vec4 worldPos = u0_model * vec4(xPos, yPos, zPos, 1.0);
    vec4 positionRelativeToCam = u1_view * worldPos;
    gl_Position = u2_projection * positionRelativeToCam;

    float xTex = float((a_data >> 5u) & 0x1Fu);
    float yTex = float(a_data & 0x1Fu);
    v_texCoords = vec2(xTex / 16.0, yTex / 16.0);

    v_light = light[(a_data >> 28u) & 0x3u];
}
