struct VertexType
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

struct ShadowPayload
{
    bool hit;
};

[shader("miss")]
void ShadowMiss(inout ShadowPayload payload)
{
    payload.hit = false;
}

cbuffer MaterialBuffer : register(b0)
{
    float4 diffuse_color;
    float material_specular_exponent;
    float material_reflectance;
    int use_diffuse_texture;
    int use_normal_map;
    int use_specular_map;
    int use_mask_texture;
}


StructuredBuffer<VertexType> vertices       : register(t0);
StructuredBuffer<uint> indices              : register(t1);
Texture2D material_mask_texture             : register(t2);
SamplerState linear_wrap : register(s0);

[shader("anyhit")]
void ShadowAnyHit(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);

    // Vertex unpacking
    int triangle_id = 3 * PrimitiveIndex();
    VertexType vertex0 = vertices[indices[triangle_id + 0]];
    VertexType vertex1 = vertices[indices[triangle_id + 1]];
    VertexType vertex2 = vertices[indices[triangle_id + 2]];

    float2 uv = vertex0.uv * barycentrics.x + vertex1.uv * barycentrics.y + vertex2.uv * barycentrics.z;

    float mask = material_mask_texture.SampleLevel(linear_wrap, uv, 0).x;

    if (mask > 0.5)
    {
        payload.hit = true;
        AcceptHitAndEndSearch();
    }
    else
        IgnoreHit();
}

[shader("closesthit")]
void ShadowCHS(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    payload.hit = true;
}