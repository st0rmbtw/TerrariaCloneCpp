#version 450 core

layout(location = 0) out vec4 frag_color;

layout(binding = 3) uniform texture2D u_background_texture;
layout(binding = 4) uniform sampler u_background_sampler;

layout(binding = 5) uniform texture2D u_world_texture;
layout(binding = 6) uniform sampler u_world_sampler;

layout(binding = 7) uniform texture2D u_lightmap_texture;
layout(binding = 8) uniform sampler u_lightmap_sampler;

layout(binding = 9) uniform texture2D u_light_texture;
layout(binding = 10) uniform sampler u_light_sampler;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec2 v_light_uv;

vec4 blend(vec4 foreground, vec4 background) {
    return foreground * foreground.a + background * (1.0 - foreground.a);
}

void main() {
    float lightmap = texture(sampler2D(u_lightmap_texture, u_light_sampler), v_light_uv).r;
    vec4 light = vec4(texture(sampler2D(u_light_texture, u_light_sampler), v_light_uv).rgb, 1);

    vec4 final_light = max(light, vec4(lightmap, lightmap, lightmap, 1.0));

    vec4 background = texture(sampler2D(u_background_texture, u_background_sampler), v_uv);
    vec4 world = texture(sampler2D(u_world_texture, u_world_sampler), v_uv) * final_light;

    frag_color = blend(world, background);
}