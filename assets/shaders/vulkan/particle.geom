#version 450 core

layout(binding = 1) uniform GlobalUniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
} ubo;

layout(location = 0) in VS_OUT {
    mat4 transform;
    vec2 uv;
    vec2 tex_size;
} gs_in[];

layout(location = 0) out vec2 g_uv;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

void main() {
    mat4 transform = gs_in[0].transform;
    vec2 uv = gs_in[0].uv;
    vec2 tex_size = gs_in[0].tex_size;
    
    mat4 mvp = ubo.view_projection * transform;

    vec2 position = vec2(0.0, 0.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = 1.0;
    g_uv = uv;
    EmitVertex();

    position = vec2(1.0, 0.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = 1.0;
    g_uv = uv + vec2(tex_size.x, 0.0);
    EmitVertex();

    position = vec2(0.0, 1.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = 1.0;
    g_uv = uv + vec2(0.0, tex_size.y);
    EmitVertex();

    position = vec2(1.0, 1.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = 1.0;
    g_uv = uv + tex_size;
    EmitVertex();

    EndPrimitive();
}