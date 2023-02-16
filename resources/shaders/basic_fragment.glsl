#version 330 core
out vec4 color;

in vec2 v_texCoords;

uniform sampler2D u3_texture;

void main() {
    color = texture(u3_texture, v_texCoords);
}
