Texture2D tex : register(t0);
SamplerState linear_clamp : register(s0);

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PS(PSInput input) : SV_TARGET
{
	return tex.Sample(linear_clamp, input.uv); // Backbuffer has srgb format so no need for calculations
}