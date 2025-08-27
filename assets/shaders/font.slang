struct VSOutput
{
    float4 position : SV_Position;
    
    float2 uv : UV;
    nointerpolation float3 color : Color;
};

[vk::binding(3)]
Texture2D Texture : register(t3);

[vk::binding(4)]
SamplerState Sampler : register(s4);

static const float GLYPH_CENTER = 0.5;
static const float OUTLINE = 0.2;

[shader("fragment")]
float4 PS(VSOutput inp) : SV_Target
{
    const float dist = Texture.Sample(Sampler, inp.uv).r;
    const float width = fwidth(dist);
    const float alpha = smoothstep(GLYPH_CENTER - OUTLINE - width, GLYPH_CENTER - OUTLINE + width, abs(dist));

    const float mu = smoothstep(0.5 - width, 0.5 + width, abs(dist));
    const float3 rgb = lerp(float3(0.0, 0.0, 0.0), inp.color, mu);
    const float4 color = float4(rgb, alpha);

    if (color.a < 0.05f)
        discard;

    return color;
};