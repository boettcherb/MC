#version 330 core

in vec2 v_texCoords;
in float v_light;

uniform sampler2D u3_texture;

void main() {
    vec4 tex = texture(u3_texture, v_texCoords);
    if (tex.a == 0.0) {
        discard;
    }
    gl_FragColor = vec4(tex.xyz * v_light, tex.w);
}
