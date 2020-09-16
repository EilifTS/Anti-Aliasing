#include "phong.hlsli"

Texture2D diffuse_texture : register(t0);
Texture2D normal_texture : register(t1);
SamplerState point_clamp : register(s0);

cbuffer LightBuffer : register(b1)
{
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
	float specular_exponent = diffuse_texture.Sample(point_clamp, input.uv).a * 255.0;
	float3 normal = normal_texture.Sample(point_clamp, input.uv).xyz;
	float distance = normal_texture.Sample(point_clamp, input.uv).w;

	float3 view_position = input.view_position * distance;
	float3 view_dir = normalize(view_position);

	float diffuse = phong_diffuse_light(light_dir.xyz, normal);
	float3 reflection_vector = phong_reflection_vector(light_dir.xyz, normal);
	float specular = phong_specular_light(reflection_vector, view_dir, specular_exponent);

	color = color * (0.2 + 0.8 * diffuse) + 0.2 * specular.xxx;

	//return float4(float3(normal.xy, normal.z), 1.0);
	return float4(color, 1.0);
}