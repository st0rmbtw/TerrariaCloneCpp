#version 450 core

layout(binding = 1) uniform UniformBuffer {
    mat4 screen_projection;
    mat4 view_projection;
} ubo;

layout(location = 0) in VS_OUT {
    mat4 transform;
    vec4 uv_offset_scale;
    vec4 color;
    vec4 outline_color;
    float outline_thickness;
    float order;
    int has_texture;
    int is_ui;
} gs_in[];

layout(location = 0) out vec2 g_uv;
layout(location = 1) flat out vec4 g_color;
layout(location = 2) flat out vec4 g_outline_color;
layout(location = 3) flat out float g_outline_thickness;
layout(location = 4) flat out int g_has_texture;

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

void main() {
    vec4 uv_offset_scale = gs_in[0].uv_offset_scale;
    vec4 color = gs_in[0].color;
    vec4 outline_color = gs_in[0].outline_color;
    float outline_thickness = gs_in[0].outline_thickness;
    float order = gs_in[0].order;
    mat4 transform = gs_in[0].transform;
    int is_ui = gs_in[0].is_ui;
    int has_texture = gs_in[0].has_texture;
    
    mat4 mvp = is_ui > 0.5 ? ubo.screen_projection * transform : ubo.view_projection * transform;

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