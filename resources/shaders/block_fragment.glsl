#version 330 core

in vec2 v_texCoords;
in float v_light;

uniform sampler2D u3_texture;

void main() {
    vec4 tex = texture(u3_texture, v_texCoords);
    gl_FragColor = vec4(vec3(tex) * v_light, tex.w);
}