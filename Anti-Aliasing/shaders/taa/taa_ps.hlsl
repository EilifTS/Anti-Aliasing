Texture2D new_sample_buffer : register(t0);
Texture2D history_buffer : register(t1);
SamplerState linear_clamp : register(s0);

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PS(PSInput input) : SV_TARGET
{
	float4 new_sample = new_sample_buffer.Sample(linear_clamp, input.uv);
	float4 history = history_buffer.Sample(linear_clamp, input.uv);

	return float4((0.13 * new_sample + (1.0 - 0.13) * history).rgb, 1.0);
}