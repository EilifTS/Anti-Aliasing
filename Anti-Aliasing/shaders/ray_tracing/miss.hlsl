
struct RayPayload
{
    float3 color;
    int depth;
};

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    // Small sun
    float3 l = -normalize(float3(0.15f, -1.0f, 0.15f));
    float sunfactor = dot(WorldRayDirection(), l);
    sunfactor = saturate(pow(sunfactor, 1000));
    float3 sky_color = float3(0.117f, 0.565f, 1.0f);
    float3 sun_color = float3(1.0, 1.0, 1.0);
    payload.color = lerp(sky_color, sun_color, sunfactor);
}