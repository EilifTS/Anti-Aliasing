cbuffer constants : register(b0)
{
	int current_sample_index;
}

Texture2D new_sample_buffer : register(t0);
Texture2D accumulated_sample_buffer : register(t1);

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
};

float4 PS(PSInput input) : SV_TARGET
{
	int2 pixel_pos = (int2)input.position.xy;
	float3 new_sample = new_sample_buffer.Load(int3(pixel_pos, 0)).xyz;
	float3 accumulated_sample = accumulated_sample_buffer.Load(int3(pixel_pos, 0)).xyz;

	int N = current_sample_index + 1;
	float r = 1.0 / (float)N;
	float t = 1.0 - r;
	
	return float4(r * new_sample + t * accumulated_sample, 1.0);
}