#include "g_buffer.hlsli"

cbuffer MaterialBuffer : register(b1)
{
	float4 diffuse_color;
	float specular_exponent;
	bool use_diffuse_texture;
}
Texture2D material_diffuse_color_texture : register(t0);
Texture2D material_normal_map_texture : register(t1);
SamplerState linear_wrap : register(s0);

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float distance : TEXCOORD1;
	float3 normal : NORMAL0;
	float3x3 tbn : NORMAL1;
};

GBuffer PS(PSInput input)
{
	GBuffer g_buffer;

	float3 color = diffuse_color;
	if (use_diffuse_texture)
		color = material_diffuse_color_texture.Sample(linear_wrap, input.uv).rgb;
	g_buffer.diffuse = float4(color, specular_exponent / 255.0);

	float3 normal_sample = material_normal_map_texture.Sample(linear_wrap, input.uv).xyx;
	float3 normal = float3(normal_sample.xy * 2.0 - 1.0, normal_sample.z);
	//float3 normal = float3(1.0, 0.0, 0.0);
	normal = normalize(mul(normal, input.tbn));

	g_buffer.normal = float4(normal, input.distance);

	return g_buffer;
}