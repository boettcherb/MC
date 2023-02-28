#version 330 core

in vec2 v_texCoords;
in float v_light;
in float v_visibility;

uniform sampler2D u3_texture;
uniform vec3 u4_bgColor;

void main() {
    vec4 tex = texture(u3_texture, v_texCoords);
    if (tex.a == 0.0) {
        discard;
    }
    vec4 color = vec4(tex.xyz * v_light, tex.w);
    gl_FragColor = mix(vec4(u4_bgColor, 1.0), color, v_visibility);
}
