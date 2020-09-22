Texture2D new_sample_buffer : register(t0);
Texture2D history_buffer : register(t1);
Texture2D depth_buffer : register(t2);
SamplerState linear_clamp : register(s0);

cbuffer TAABuffer : register(b1)
{
	matrix view_to_prev_frame_clip_matrix;
};

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float3 view_position : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

float4 PS(PSInput input) : SV_TARGET
{
	float distance = depth_buffer.Sample(linear_clamp, input.uv).w;
	float3 view_position = input.view_position * distance;

	float4 prev_frame_pos = mul(float4(view_position, 1.0), view_to_prev_frame_clip_matrix);
	prev_frame_pos /= prev_frame_pos.w;
	float2 prev_frame_uv = prev_frame_pos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	float4 new_sample = new_sample_buffer.Sample(linear_clamp, input.uv);

	float alpha = 1.0;
	float4 history = float4(0.0, 0.0, 0.0, 0.0);
	if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0)
	{
		history = history_buffer.Sample(linear_clamp, prev_frame_uv);
		alpha = 0.13;
	}

	//return new_sample;
	//return float4(prev_frame_uv, 1.0f, 1.0f);
	//return float4(motion_vectors.Sample(linear_clamp, input.uv) * 1000.0f, 0.0f, 1.0f);
	return float4(lerp(history.rgb, new_sample.rgb, alpha), 1.0);
}