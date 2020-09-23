Texture2D tex : register(t0);
Texture2D<float2> motion_vectors : register(t1);
SamplerState linear_clamp : register(s0);

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PS(PSInput input) : SV_TARGET
{
	// Code to display motion vectors
	//float2 motion_vector = motion_vectors.Sample(linear_clamp, input.uv);
	//return float4(motion_vector * 1000.0, 0.0, 1.0);

	return tex.Sample(linear_clamp, input.uv); // Converting from 8bit to 16bit
}