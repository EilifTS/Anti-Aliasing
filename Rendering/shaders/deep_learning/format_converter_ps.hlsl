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
	//return float4(tex.Sample(linear_clamp, input.uv).aaa, 1.0); // Show accum buffer
	return float4(pow(tex.Sample(linear_clamp, input.uv).rgb, 2.2), 1.0); // Converting from 8bit to 16bit and getting the correct gamma
}