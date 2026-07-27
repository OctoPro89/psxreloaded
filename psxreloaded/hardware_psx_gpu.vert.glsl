#version 330 core
layout (location = 0) in vec2 a_pos;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in uvec2 a_extra;
layout (location = 4) in uvec4 a_extra2;

out vec3 v_color;
out vec2 v_uv;
flat out uvec2 v_extra;
flat out uvec4 v_extra2;
uniform vec2 u_resolution;

void main() {
    vec2 pos = a_pos;

    // PS1 -> NDC (top-left origin)
    vec2 ndc = vec2(
        (pos.x / u_resolution.x) * 2.0 - 1.0,
        1.0 - (pos.y / u_resolution.y) * 2.0
    );

    gl_Position = vec4(ndc, 0.0, 1.0);

    v_color = a_color;
    v_uv = a_uv;
    v_extra = a_extra;
    v_extra2 = a_extra2;
}