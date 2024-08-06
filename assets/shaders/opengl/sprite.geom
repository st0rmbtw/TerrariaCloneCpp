#version 330 core

uniform UniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
    mat4 nonscale_view_projection;
} ubo;

in VS_OUT {
    mat4 transform;
    vec4 uv_offset_scale;
    vec4 color;
    vec4 outline_color;
    float outline_thickness;
    float order;
    int has_texture;
    bool is_ui;
    bool is_nonscale;
} gs_in[];

out vec2 g_uv;
flat out vec4 g_color;
flat out vec4 g_outline_color;
flat out float g_outline_thickness;
flat out int g_has_texture;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

void main() {
    mat4 transform = gs_in[0].transform;
    vec4 uv_offset_scale = gs_in[0].uv_offset_scale;
    vec4 color = gs_in[0].color;
    vec4 outline_color = gs_in[0].outline_color;
    float outline_thickness = gs_in[0].outline_thickness;
    float order = gs_in[0].order;
    bool is_ui = gs_in[0].is_ui;
    bool is_nonscale = gs_in[0].is_nonscale;
    int has_texture = gs_in[0].has_texture;
    
    mat4 mvp = is_ui ? ubo.screen_projection * transform : is_nonscale ? ubo.nonscale_view_projection * transform : ubo.view_projection * transform;

    vec2 position = vec2(0.0, 0.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = order;
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_outline_color = outline_color;
    g_outline_thickness = outline_thickness;
    g_has_texture = has_texture;
    EmitVertex();

    position = vec2(1.0, 0.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = order;
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_outline_color = outline_color;
    g_outline_thickness = outline_thickness;
    g_has_texture = has_texture;
    EmitVertex();

    position = vec2(0.0, 1.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = order;
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_outline_color = outline_color;
    g_outline_thickness = outline_thickness;
    g_has_texture = has_texture;
    EmitVertex();

    position = vec2(1.0, 1.0);
    gl_Position = mvp * vec4(position, 0.0, 1.0);
    gl_Position.z = order;
    g_uv = position * uv_offset_scale.zw + uv_offset_scale.xy;
    g_color = color;
    g_outline_color = outline_color;
    g_outline_thickness = outline_thickness;
    g_has_texture = has_texture;
    EmitVertex();

    EndPrimitive();
}