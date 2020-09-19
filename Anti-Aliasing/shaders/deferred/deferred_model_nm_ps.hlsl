#include "g_buffer.hlsli"

cbuffer MaterialBuffer : register(b2)
{
	float4 diffuse_color;
	float material_specular_exponent;
	int use_diffuse_texture;
	int use_normal_map;
	int use_specular_map;
	int use_mask_texture;
}
Texture2D material_diffuse_color_texture : register(t0);
Texture2D material_normal_map_texture : register(t1);
Texture2D material_specular_map_texture : register(t2);
Texture2D material_mask_texture : register(t3);
SamplerState linear_wrap : register(s0);

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float distance : TEXCOORD1;
	float3x3 tbn : NORMAL;
};

GBuffer PS(PSInput input)
{
	GBuffer g_buffer;

	if (use_mask_texture == 1)
	{
		float mask = material_mask_texture.Sample(linear_wrap, input.uv).x;
		clip(mask - 0.5);
	}

	float3 color = float3(0.0, 0.0, 0.0);
	if (use_diffuse_texture == 1)
		color = material_diffuse_color_texture.Sample(linear_wrap, input.uv).rgb;
	else
		color = diffuse_color;
	
	float specular_intensity = 0.0;
	if (use_specular_map == 1)
		specular_intensity = material_specular_map_texture.Sample(linear_wrap, input.uv).x;
	else
		specular_intensity = 0.00;

	float3 normal = float3(0.0, 0.0, 1.0);
	if (use_normal_map == 1)
	{
		float3 normal_sample = material_normal_map_texture.Sample(linear_wrap, input.uv).xyx;
		normal = float3((normal_sample.xy * 2.0 - 1.0) * float2(1.0, -1.0), normal_sample.z);
		normal = normalize(mul(normal, input.tbn));
	}
	else
	{
		normal = input.tbn._m20_m21_m22;
	}

	g_buffer.diffuse = float4(color, specular_intensity);
	g_buffer.normal = float4(normal, input.distance);

	return g_buffer;
}