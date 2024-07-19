#version 450

#extension GL_EXT_buffer_reference : require

struct Vertex {
    vec3 position;
    float uv_x;
    vec3 normal;
    float uv_y;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
    vec3 pos;
} camera_data;

struct Grass {
    vec3 position;
    float tilt;
    vec2 facing;
    float size;
    float bend;
};

layout(set = 1, binding = 0, std430) buffer GrassInstanceBuffer {
    Grass grasses[];
};

layout(push_constant) uniform constants {
    mat4 transform;
    VertexBuffer vertex_buffer;
    float time;
} pc;

layout(location = 0) out vec3 v_frag_pos;
layout(location = 1) out vec2 v_tex_coord;
layout(location = 2) out vec3 v_normal;

mat3 get_y_rotation_matrix(float p_angle) {
    float c = cos(p_angle);
    float s = sin(p_angle);
    return mat3(
        c, 0, s,
        0, 1, 0,
        -s, 0, c
    );
}

void main() {
    Vertex vertex = pc.vertex_buffer.vertices[gl_VertexIndex];
    Grass grass = grasses[gl_InstanceIndex];

    vec3 position = get_y_rotation_matrix(grass.tilt) * vertex.position;

    vec3 frag_pos = position + grass.position;
    frag_pos = (pc.transform * vec4(frag_pos, 1.0)).xyz;

    vec3 bend = vec3(grass.facing.x, 0, grass.facing.y);
    bend *= grass.bend / 4.0;

    // top one should be bended more
    float index_multiplier = 0;
    // 0
    // 3 4
    // 5 6
    // 1 2
    switch (gl_VertexIndex) {
        case 0:
        index_multiplier = 1.0;
        break;
        case 3:
        case 4:
        index_multiplier = 0.66;
        break;
        case 5:
        case 6:
        index_multiplier = 0.33;
        break;
        case 1:
        case 2:
        index_multiplier = 0.0;
        break;
    }

    frag_pos += bend * index_multiplier;

    float wind_strength = 0.2;
    float wind_speed = 1.0;
    if (index_multiplier <= 0.1) {
        wind_speed = 0.0;
    }

    float wind_wave = sin(pc.time * wind_speed + position.x * 0.1 + position.z * 0.1)
            * wind_strength;

    frag_pos += vec3(wind_wave, 0.0, wind_wave) * index_multiplier;

    gl_Position = camera_data.proj
            * camera_data.view
            * vec4(frag_pos, 1.0);

    v_frag_pos = frag_pos;
    v_tex_coord = vec2(vertex.uv_x, vertex.uv_y);
    v_normal = vertex.normal;
}
