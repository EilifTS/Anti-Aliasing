RaytracingAccelerationStructure scene : register(t0);
RWTexture2D<float4> output : register(u0);


[shader("raygeneration")]
void RayGenerationShader()
{
    uint3 index = DispatchRaysIndex();
    float3 color = float3(0.4, 0.6, 0.2);
    output[index.xy] = float4(color, 1);
}

struct Payload
{
    bool hit;
};

[shader("miss")]
void MissShader(inout Payload payload)
{
    payload.hit = false;
}

[shader("closesthit")]
void ClosestHitShader(inout Payload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}