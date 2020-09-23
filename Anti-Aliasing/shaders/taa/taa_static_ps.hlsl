Texture2D history_buffer : register(t0);
Texture2D depth_buffer : register(t1);
SamplerState linear_clamp : register(s0);

cbuffer constants : register(b0)
{
	float2 window_size;
	float2 rec_window_size;
}

cbuffer TAABuffer : register(b2)
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

float3 catmulSample(float2 uv)
{
	return history_buffer.Sample(linear_clamp, uv).rgb;
}

float3 catmulRom(float2 uv)
{
	float2 position = uv * window_size;
	float2 center_position = floor(position - 0.5) + 0.5;
	float2 f = position - center_position;
	float2 f2 = f * f;
	float2 f3 = f2 * f;

	float2 w0 = -0.5 * f3 + f2 - 0.5 * f;
	float2 w1 = 1.5 * f3 - 2.5 * f2 + 1.0;
	float2 w2 = -1.5 * f3 + 2.0 * f2 + 0.5 * f;
	float2 w3 = 0.5 * f3 - 0.5 * f2;
	// float2 w2 = 1.0 - w0 - w1 - w3

	float2 w12 = w1 + w2;
	float2 tc12 = rec_window_size * (center_position + w2 / w12);
	float3 center_color = catmulSample(tc12);

	float2 tc0 = rec_window_size * (center_position - 1.0);
	float2 tc3 = rec_window_size * (center_position + 2.0);
	float4 color = 
		float4(catmulSample(float2(tc12.x, tc0.y)), 1.0) * (w12.x * w0.y) + 
		float4(catmulSample(float2(tc0.x, tc12.y)), 1.0) * (w0.x * w12.y) +
		float4(center_color			              , 1.0) * (w12.x * w12.y) +
		float4(catmulSample(float2(tc3.x, tc12.y)), 1.0) * (w3.x * w12.y) +
		float4(catmulSample(float2(tc12.x, tc3.y)), 1.0) * (w12.x * w3.y);

	return color.rgb / color.a;
}

float4 PS(PSInput input) : SV_TARGET
{
	float distance = depth_buffer.Sample(linear_clamp, input.uv).w;
	float3 view_position = input.view_position * distance;

	float4 prev_frame_pos = mul(float4(view_position, 1.0), view_to_prev_frame_clip_matrix);
	prev_frame_pos /= prev_frame_pos.w;
	float2 prev_frame_uv = prev_frame_pos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

	float4 history = float4(0.0, 0.0, 0.0, 0.0);
	if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0)
		history = float4(catmulRom(prev_frame_uv), 1.0);
		//history = history_buffer.Sample(linear_clamp, prev_frame_uv);

	//return new_sample;
	//return float4(prev_frame_uv, 1.0f, 1.0f);
	//return float4(motion_vectors.Sample(linear_clamp, input.uv) * 1000.0f, 0.0f, 1.0f);
	return history;
}