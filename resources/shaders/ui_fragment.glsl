#version 330 core

in vec2 v_texCoords;

uniform sampler2D u3_texture;

void main() {
    gl_FragColor = texture(u3_texture, v_texCoords);
    if (gl_FragColor.a == 0.0) {
        discard;
    }
}
