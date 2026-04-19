struct GlobalUniforms
{
    float4x4 screen_projection;
    float4x4 view_projection;
    float4x4 inv_view_projection;
    float4x4 nonscale_view_projection;
    float4x4 nonscale_projection;
    float2 camera_position;
    float2 window_size;
};