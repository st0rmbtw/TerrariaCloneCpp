#version 330 core

uniform UniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
} ubo;

in VS_OUT {
    mat4 transform;
    vec4 uv_offset_scale;
    flat vec4 color;
    flat int has_texture;
    flat int is_ui;
} gs_in[];

out vec2 g_uv;
flat out vec4 g_color;
flat out int g_has_texture;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

void main() {
    vec4 uv_offset_scale = gs_in[0].uv_offset_scale;
    vec4 color = gs_in[0].color;
    mat4 transform = gs_in[0].transform;
    int is_ui = gs_in[0].is_ui;
    int has_texture = gs_in[0].has_texture;
    
    mat4 mvp = is_ui > 0 ? ubo.screen_projection * transform : ubo.view_projection * transform;

    vec2 position = vec2(0.0, 0.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_has_texture = has_texture;
    EmitVertex();

    position = vec2(1.0, 0.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_has_texture = has_texture;
    EmitVertex();

    position = vec2(0.0, 1.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_has_texture = has_texture;
    EmitVertex();

    position = vec2(1.0, 1.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_has_texture = has_texture;
    EmitVertex();

    EndPrimitive();
}