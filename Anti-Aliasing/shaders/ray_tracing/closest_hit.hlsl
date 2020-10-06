struct VertexType
{
    float3 position;
    float3 normal;
    float4 tangent;
    float2 uv;
};

struct RayPayload
{
    float3 color;
    int depth;
};

struct ShadowPayload
{
    bool hit;
};

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
Texture2D material_diffuse_color_texture    : register(t2);
Texture2D material_normal_map_texture       : register(t3);
Texture2D material_specular_map_texture     : register(t4);
Texture2D material_mask_texture             : register(t5);
SamplerState linear_wrap : register(s0);
RaytracingAccelerationStructure rtscene : register(t6);
// Copy paste from illumination.hlsli
float specular_spec(float3 n, float3 l, float3 v, float3 h, float r)
{
    float alpha = r * r;
    float alpha2 = alpha * alpha;

    const float pi = 3.14159265f;
    float k = (r + 1.0) * (r + 1.0) * 0.125;
    float nh = dot(n, h);
    float nv = dot(n, v);
    float nl = dot(n, l);
    float vh = dot(v, h);


    // D_gxx
    float t = nh * nh * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (pi * t * t);
    return D;

    // G_schlick
    float G_v = 1.0 / (nv * (1.0 - k) + k);
    float G_l = 1.0 / (nl * (1.0 - k) + k);
    float G = G_v * G_l;

    // Fresnel
    float F_0 = 0.04;
    float power = (-5.55473 * vh - 6.98316) * vh;
    float F = F_0 + ldexp(1.0 - F_0, power);

    return D * G * F * 0.25;
}

bool TraceShadowRay(float3 pos, float3 dir)
{
    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = dir;

    ray.TMin = 0.01;
    ray.TMax = 100000;

    ShadowPayload new_payload;
    TraceRay(rtscene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 1, 0, 1, ray, new_payload);
    return new_payload.hit;
}

float3 TraceReflectionRay(float3 pos, float3 dir, int depth)
{
    RayDesc ray;
    ray.Origin = pos;
    ray.Direction = dir;

    ray.TMin = 0.01;
    ray.TMax = 100000;

    RayPayload payload;
    payload.depth = depth;
    TraceRay(rtscene, 0, 0xFF, 0, 0, 0, ray, payload);
    return payload.color;
}

[shader("closesthit")]
void ClosestHitShader(inout RayPayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    float3 barycentrics = float3(1.0 - attribs.barycentrics.x - attribs.barycentrics.y, attribs.barycentrics.x, attribs.barycentrics.y);

    // Vertex unpacking
    int triangle_id = 3 * PrimitiveIndex();
    VertexType vertex0 = vertices[indices[triangle_id + 0]];
    VertexType vertex1 = vertices[indices[triangle_id + 1]];
    VertexType vertex2 = vertices[indices[triangle_id + 2]];

    float3 ms_normal = vertex0.normal * barycentrics.x + vertex1.normal * barycentrics.y + vertex2.normal * barycentrics.z;
    float4 ms_tangent = vertex0.tangent * barycentrics.x + vertex1.tangent * barycentrics.y + vertex2.tangent * barycentrics.z;
    float2 uv = vertex0.uv * barycentrics.x + vertex1.uv * barycentrics.y + vertex2.uv * barycentrics.z;

    float3 ws_position = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 ws_normal = normalize(mul(ms_normal, (float3x3)ObjectToWorld4x3()));
    float3 ws_tangent = normalize(mul(ms_tangent.xyz, (float3x3)ObjectToWorld4x3()));
    float3 ws_bitangent = cross(ws_tangent, ws_normal) * ms_tangent.w;

    float3 view_direction = -normalize(WorldRayDirection());

    // Calculate normal
    float3 n = ws_normal;
    if (use_normal_map == 1)
    {
        float3 normal_sample = material_normal_map_texture.SampleLevel(linear_wrap, uv, 0).xyx;
        normal_sample = float3((normal_sample.xy * 2.0 - 1.0) * float2(1.0, -1.0), normal_sample.z);
        float3x3 tbn = float3x3(ws_tangent, ws_bitangent, ws_normal);
        n = normalize(mul(normal_sample, tbn));
    }

    // Get material color
    float3 albedo_color = float3(0.0, 0.0, 0.0);
    if (use_diffuse_texture == 1)
    {
        albedo_color = material_diffuse_color_texture.SampleLevel(linear_wrap, uv, 0).rgb;
    }

    // Get specular intentisy
    float specular_intensity = 0.0;
    if (use_specular_map == 1)
        specular_intensity = material_specular_map_texture.SampleLevel(linear_wrap, uv, 0).x;

    // Light calculation
    float3 color = 0.2 * albedo_color;
    float3 l = -normalize(float3(0.15f, -1.0f, 0.15f));
    float3 v = view_direction;

    // Trace shadows
    bool in_shadow = TraceShadowRay(ws_position, l);
    float shadow_factor = in_shadow ? 0.0 : 1.0;


    if (shadow_factor > 0.0)
    {
        // Empirical model
        float roughness = 0.8 - 0.7 * specular_intensity;
        float shinyness = 0.0 + 0.2 * specular_intensity;

        float3 h = normalize(l + v);
        float nl = dot(n, l);

        float diffuse = 1.0f;

        float specular = specular_spec(n, l, v, h, roughness);
        
        color += saturate(nl) * shadow_factor * (lerp(diffuse * albedo_color, specular, shinyness));
    }


    // Trace reflection
    if (payload.depth < 2 && material_reflectance > 0.0)
    {
        float3 reflected_color = TraceReflectionRay(ws_position, -(v - 2 * dot(v, n) * n), payload.depth + 1);
        color = lerp(color, reflected_color, material_reflectance * specular_intensity);
    }

    payload.color = color;
}