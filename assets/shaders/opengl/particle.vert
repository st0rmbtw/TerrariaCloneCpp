#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec4 a_rotation;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec2 a_tex_size;
layout(location = 4) in float a_scale;

out VS_OUT {
    flat mat4 transform;
    flat vec2 uv;
    flat vec2 tex_size;
} vs_out;

const vec2 PARTICLE_SIZE = vec2(8, 8);

void main() {
    float qxx = a_rotation.x * a_rotation.x;
    float qyy = a_rotation.y * a_rotation.y;
    float qzz = a_rotation.z * a_rotation.z;
    float qxz = a_rotation.x * a_rotation.z;
    float qxy = a_rotation.x * a_rotation.y;
    float qyz = a_rotation.y * a_rotation.z;
    float qwx = a_rotation.w * a_rotation.x;
    float qwy = a_rotation.w * a_rotation.y;
    float qwz = a_rotation.w * a_rotation.z;

    mat4 rotation_matrix = mat4(
        vec4(1.0 - 2.0 * (qyy + qzz), 2.0 * (qxy + qwz), 2.0 * (qxz - qwy), 0.0),
        vec4(2.0 * (qxy - qwz), 1.0 - 2.0 * (qxx +  qzz), 2.0 * (qyz + qwx), 0.0),
        vec4(2.0 * (qxz + qwy), 2.0 * (qyz - qwx), 1.0 - 2.0 * (qxx +  qyy), 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    );

    mat4 transform = mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(a_position, 0.0, 1.0)
    );

    transform *= rotation_matrix;

    vec2 size = PARTICLE_SIZE * a_scale;
    vec2 offset = -0.5 * size;

    // translate
    transform[3] = transform[0] * offset[0] + transform[1] * offset[1] + transform[2] * 0.0 + transform[3];

    // scale
    transform[0] = transform[0] * size[0];
    transform[1] = transform[1] * size[1];

    vs_out.transform = transform;
    vs_out.uv = a_uv / a_tex_size;
    vs_out.tex_size = PARTICLE_SIZE / a_tex_size;

    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}