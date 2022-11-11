#version 330 core
layout(location = 0) in uint a_data;

out vec2 v_texCoords;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main() {
    // retrieve the x, y, and z positions from their place in the data
    float xPos = float((a_data >> 23u) & 0x1Fu);
    float yPos = float((a_data >> 15u) & 0xFFu);
    float zPos = float((a_data >> 10u) & 0x1Fu);
    gl_Position = u_projection * u_view * u_model * vec4(xPos, yPos, zPos, 1.0f);

    // retrieve the tex coords from their place in the data
    float xTex = float((a_data >> 5u) & 0x1Fu);
    float yTex = float(a_data & 0x1Fu);
    v_texCoords = vec2(xTex / 16.0f, yTex / 16.0f);
}
