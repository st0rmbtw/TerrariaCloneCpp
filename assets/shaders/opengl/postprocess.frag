#version 330 core

out vec4 frag_color;

uniform sampler2D u_background_texture;
uniform sampler2D u_world_texture;
uniform sampler2D u_lightmap_texture;
uniform sampler2D u_light_texture;

in vec2 v_uv;
in vec2 v_light_uv;

vec4 blend(vec4 foreground, vec4 background) {
    return foreground * foreground.a + background * (1.0 - foreground.a);
}

void main() {
    float lightmap = texture(u_lightmap_texture, v_light_uv).r;
    vec4 light = vec4(texture(u_light_texture, v_light_uv).rgb, 1);

    vec4 final_light = max(light, vec4(lightmap, lightmap, lightmap, 1.0));

    vec4 background = texture(u_background_texture, v_uv);
    vec4 world = texture(u_world_texture, v_uv) * final_light;

    frag_color = blend(world, background);
}