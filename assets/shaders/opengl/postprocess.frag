#version 330 core

out vec4 frag_color;

uniform sampler2D u_background_texture;
uniform sampler2D u_world_texture;
uniform sampler2D u_light_texture;

in vec2 v_uv;
in vec2 v_light_uv;

vec4 blend(vec4 foreground, vec4 background) {
    return foreground * foreground.a + background * (1.0 - foreground.a);
}

void main() {
    vec4 light = vec4(texture2D(u_light_texture, v_light_uv).rgb, 1);
    vec4 background = texture2D(u_background_texture, v_uv);
    vec4 world = texture2D(u_world_texture, v_uv) * light;

    frag_color = blend(world, background);
}