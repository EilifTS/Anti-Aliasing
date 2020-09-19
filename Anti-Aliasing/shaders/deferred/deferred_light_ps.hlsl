#include "phong.hlsli"
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
}

struct PSInput
{
	float4 position : SV_POSITION;
	float3 view_position : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

float4 PS(PSInput input) : SV_TARGET
{
	float3 color = diffuse_texture.Sample(point_clamp, input.uv).rgb;
	float specular_intensity = diffuse_texture.Sample(point_clamp, input.uv).a;
	float3 normal = normal_texture.Sample(point_clamp, input.uv).xyz;
	float distance = normal_texture.Sample(point_clamp, input.uv).w;

	float3 view_position = input.view_position * distance;
	float3 view_dir = normalize(view_position);

	// Shadow calculation
	float shadow_factor = calculateShadowSmooth4(shadowmap, shadow_sampler, view_position, view_to_shadowmap_matrix);

	// Light calculation
	float diffuse = 0.0;
	float specular = 0.0;
	if (shadow_factor > 0.0)
	{
		diffuse = phong_diffuse_light(light_dir.xyz, normal) * shadow_factor;
		float3 reflection_vector = phong_reflection_vector(light_dir.xyz, normal);
		specular = phong_specular_light(reflection_vector, view_dir, 10) * shadow_factor;
	}
		
	color = color * (0.2 +  0.8 * diffuse) + 0.8 * specular_intensity * specular.xxx;


	//return float4(float3(normal.xy, normal.z), 1.0);
	//return float4(shadowmap.Sample(point_clamp, input.uv).xxx, 1.0);
	return float4(color, 1.0);
}