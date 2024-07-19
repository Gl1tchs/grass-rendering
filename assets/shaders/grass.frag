#version 450

layout(location = 0) in vec3 v_frag_pos;
layout(location = 1) in vec2 v_tex_coord;
layout(location = 2) in vec3 v_normal;

layout(location = 0) out vec4 o_color;

const vec3 BOTTOM_COLOR = vec3(0.25686, 0.60784, 0.0);
const vec3 TOP_COLOR = vec3(0.76863, 0.85098, 0.0);

void main() {
    vec3 color = mix(BOTTOM_COLOR, TOP_COLOR, -v_tex_coord.x);

    o_color = vec4(color, 0.98);
}
