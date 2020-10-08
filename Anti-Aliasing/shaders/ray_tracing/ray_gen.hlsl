RaytracingAccelerationStructure rtscene : register(t0);
RWTexture2D<float4> rtoutput : register(u0);

cbuffer CameraBuffer : register(b0)
{
    matrix view_matrix;
    matrix inv_view_matrix;
    matrix projection_matrix;
    matrix inv_projection_matrix;
    matrix projection_matrix_no_jitter;
    matrix inv_projection_matrix_no_jitter;
}

cbuffer JitterBuffer : register(b1)
{
    int jitter_count;
    int sample_count;
    int jitter_index;
    int placeholder;
    float4 jitters[128];
}

struct RayPayload
{
    float3 color;
    int depth;
};

int cash(uint x, uint y)
{
    int h = 0 + x * 374761393 + y * 668265263;
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

[shader("raygeneration")]
void RayGenerationShader()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();


    uint j = (cash(launchIndex.x, launchIndex.y) + jitter_index) % jitter_count;

    float2 crd = float2(launchIndex.xy) + (jitters[j].xy * float2(1.0, -1.0) + float2(0.0, 1.0));
    float2 dims = float2(launchDim.xy);

    float2 d = ((crd / dims) * 2.0 - 1.0);
    float4 clip_pos = float4(d * float2(1.0, -1.0), 0.0f, 1.0f);

    float4 view_pos = mul(clip_pos, inv_projection_matrix_no_jitter);

    RayDesc ray;
    ray.Origin = mul(float4(0.0, 0.0, 0.0, 1.0), inv_view_matrix).xyz;
    ray.Direction = normalize(mul(view_pos.xyz, (float3x3)inv_view_matrix));

    ray.TMin = 0;
    ray.TMax = 100000;

    RayPayload payload;
    payload.depth = 1;
    TraceRay(rtscene, 0, 0xFF, 0, 0, 0, ray, payload);
    float3 col = payload.color;


    rtoutput[launchIndex.xy] = float4(col, 1.0);
    //rtoutput[launchIndex.xy] = float4(j.xxx / 128.0, 1.0);

}