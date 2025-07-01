struct VertexOut
{
    float4 position [[position]];
    float3 color    [[flat]];
    float2 tex_size [[flat]];
    float2 uv;
};

constant const float GLYPH_CENTER = 0.5;
constant const float OUTLINE = 0.5;


fragment float4 PS(
    VertexOut inp [[stage_in]],
    texture2d<float> texture [[texture(3)]],
    sampler texture_sampler [[sampler(4)]]
) {
    const float dist = texture.sample(texture_sampler, inp.uv).r;
    const float width = fwidth(dist);
    const float alpha = smoothstep(GLYPH_CENTER - OUTLINE - width, GLYPH_CENTER - OUTLINE + width, abs(dist));

    const float mu = smoothstep(0.5-width, 0.5+width, abs(dist));
    const float3 rgb = mix(float3(0.0, 0.0, 0.0), inp.color, mu);
    const float4 color = float4(rgb, alpha);

    if (color.a < 0.05) discard_fragment();

    return color;
}