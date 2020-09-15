#include "g_buffer.hlsli"

cbuffer MaterialBuffer : register(b1)
{
	float4 diffuse_color;
	float specular_exponent;
	bool use_diffuse_texture;
}
Texture2D material_diffuse_color_texture : register(t0);
SamplerState linear_wrap : register(s0);

struct PSInput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
	float distance : TEXCOORD1;
};

GBuffer PS(PSInput input)
{
	GBuffer g_buffer;

	float3 color = diffuse_color;
	if (use_diffuse_texture)
		color = material_diffuse_color_texture.Sample(linear_wrap, input.uv).rgb;
	g_buffer.diffuse = float4(color, specular_exponent / 255.0);

	g_buffer.normal = float4(normalize(input.normal), input.distance);

	return g_buffer;
}