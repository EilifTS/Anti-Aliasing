Texture2D history_buffer : register(t0);
Texture2D depth_buffer : register(t1);
Texture2D<float2> motion_vectors : register(t2);
SamplerState linear_clamp : register(s0);

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PS(PSInput input) : SV_TARGET
{
	float distance = depth_buffer.Sample(linear_clamp, input.uv).w;

	float2 prev_frame_uv = input.uv + motion_vectors.Sample(linear_clamp, input.uv);

	float4 history = float4(0.0, 0.0, 0.0, 0.0);
	if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0)
		history = history_buffer.Sample(linear_clamp, prev_frame_uv);

	//return new_sample;
	//return float4(prev_frame_uv, 1.0f, 1.0f);
	//return float4(motion_vectors.Sample(linear_clamp, input.uv) * 1000.0f, 0.0f, 1.0f);
	return history;
}