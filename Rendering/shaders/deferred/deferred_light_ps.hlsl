#include "illumination.hlsli"
#include "shadow.hlsli"

Texture2D diffuse_texture : register(t0);
Texture2D normal_texture : register(t1);
Texture2D shadowmap : register(t2);
SamplerState point_clamp : register(s0);
SamplerComparisonState shadow_sampler : register(s1);

cbuffer LightBuffer : register(b1)
{
	matrix view_to_shadowmap_matrix;
	float4 light_dir;
	float roughness_in;
}

struct PSInput
{
	float4 position : SV_POSITION;
	float3 view_position : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

float4 PS(PSInput input) : SV_TARGET
{
	float3 albedo_color = diffuse_texture.Sample(point_clamp, input.uv).rgb;
	float specular_intensity = diffuse_texture.Sample(point_clamp, input.uv).a;
	float3 normal = normal_texture.Sample(point_clamp, input.uv).xyz;
	float distance = normal_texture.Sample(point_clamp, input.uv).w;

	float3 view_position = input.view_position * distance;
	float3 view_dir = normalize(view_position);

	// Shadow calculation
	float shadow_factor = calculateShadowSmooth4(shadowmap, shadow_sampler, view_position, view_to_shadowmap_matrix);

	// Light calculation
	// Ambient light
	float3 color = 0.2 * albedo_color;

	if (shadow_factor > 0.0)
	{
		float roughness = 0.8 - 0.7 * specular_intensity;
		float shinyness = 0.0 + 0.2 * specular_intensity;

		float3 l = normalize(-light_dir.xyz);
		float3 v = -view_dir;
		float3 n = normalize(normal);
		float3 h = normalize(l + v);
		float3 r = phong_reflection_vector(l, n);
		float nl = dot(n, l);

		float diffuse = 1.0f;
		float specular = specular_spec(n, l, v, h, roughness);

		color += saturate(nl) * shadow_factor * (lerp(diffuse * albedo_color, specular.xxx, shinyness));
	}

	return float4(color, 1.0);
}