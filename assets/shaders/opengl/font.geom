#version 330 core

uniform UniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
} ubo;

in VS_OUT {
    vec3 color;
    vec2 pos;
    vec2 size;
    vec2 tex_size;
    vec2 tex_uv;
    int is_ui;
} gs_in[];

out vec2 g_uv;
flat out vec3 g_color;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

void main() {
    vec3 color = gs_in[0].color;
    vec2 position = gs_in[0].pos;
    vec2 size = gs_in[0].size;
    vec2 tex_size = gs_in[0].tex_size;
    vec2 tex_uv = gs_in[0].tex_uv;
    int is_ui = gs_in[0].is_ui;
    
    mat4 mvp = is_ui > 0 ? ubo.screen_projection : ubo.view_projection;

    gl_Position = mvp * vec4(position, 0.0, 1.0);
    g_uv = tex_uv;
    g_color = color;
    EmitVertex();

    gl_Position = mvp * vec4(position.x + size.x, position.y, 0.0, 1.0);
    g_uv = tex_uv + vec2(tex_size.x, 0.0);
    g_color = color;
    EmitVertex();

    gl_Position = mvp * vec4(position.x, position.y + size.y, 0.0, 1.0);
    g_uv = tex_uv + vec2(0.0, tex_size.y);
    g_color = color;
    EmitVertex();

    gl_Position = mvp * vec4(position + size, 0.0, 1.0);
    g_uv = tex_uv + tex_size;
    g_color = color;
    EmitVertex();

    EndPrimitive();
}