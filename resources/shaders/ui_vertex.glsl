#version 330 core

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_tex;

out vec2 v_texCoords;

void main() {
	gl_Position = vec4(a_pos, 0.0, 1.0);
	v_texCoords = a_tex;
}
