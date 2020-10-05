
struct RayPayload
{
    float3 color;
    int depth;
};

[shader("miss")]
void MissShader(inout RayPayload payload)
{
    payload.color = float3(0.117f, 0.565f, 1.0f);
}