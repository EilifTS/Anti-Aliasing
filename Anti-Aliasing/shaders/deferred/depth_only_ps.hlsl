#include "g_buffer.hlsli"

cbuffer MaterialBuffer : register(b2)
{
	float4 diffuse_color;
	float material_specular_exponent;
	float material_reflectance;
	int use_diffuse_texture;
	int use_normal_map;
	int use_specular_map;
	int use_mask_texture;
}
Texture2D material_mask_texture : register(t0);
SamplerState linear_wrap : register(s0);

struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

void PS(PSInput input)
{
	if (use_mask_texture)
	{
		float m = material_mask_texture.Sample(linear_wrap, input.uv).r;
		clip(m - 0.5);
	}
}