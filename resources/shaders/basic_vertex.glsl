#version 330 core

layout(location = 0) in uint a_data;

out vec2 v_texCoords;
out float v_light;

uniform mat4 u0_model;
uniform mat4 u1_view;
uniform mat4 u2_projection;

void main() {
    // retrieve the x, y, and z positions from their place in the data
    float xPos = float((a_data >> 23u) & 0x1Fu);
    float yPos = float((a_data >> 15u) & 0xFFu);
    float zPos = float((a_data >> 10u) & 0x1Fu);
    gl_Position = u2_projection * u1_view * u0_model * vec4(xPos, yPos, zPos, 1.0f);

    // retrieve the tex coords from their place in the data
    float xTex = float((a_data >> 5u) & 0x1Fu);
    float yTex = float(a_data & 0x1Fu);
    v_texCoords = vec2(xTex / 16.0f, yTex / 16.0f);

    // retrieve the light value
    v_light = 0.625f + 0.125f * ((a_data >> 28u) & 0x3u);
}
