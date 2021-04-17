Buffer<half> history_buffer : register(t0);
Buffer<half> cnn_res : register(t1);

Texture2D input_texture	: register(t2);
Texture2D<float> depth_buffer	: register(t3);
SamplerState linear_clamp : register(s0);

#ifndef UPSAMPLE_FACTOR
#define UPSAMPLE_FACTOR 4
#endif

cbuffer constants : register(b0)
{
	float2 window_size;
	float2 rec_window_size;
	float2 current_jitter;
}

struct PSInput
{
	float4 position : SV_POSITION;
	float2 clip_position : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

float4 PS(PSInput input) : SV_TARGET
{ 
	uint2 hr_pixel_pos = (uint2)input.position.xy; // Position in pixels of pixel in high res image
	uint2 lr_pixel_pos = hr_pixel_pos / UPSAMPLE_FACTOR; // Position of pixel in low res image
	float2 jitter_offset = current_jitter; // Pixel offset of jitter in low res image

	// JAU input
	float2 hr_jitter_pos = input.position.xy + (float2(0.5, 0.5) - jitter_offset) * UPSAMPLE_FACTOR;
	float2 hr_jitter_uv = hr_jitter_pos * rec_window_size;
	float4 jau_rgbd = float4(0.0, 0.0, 0.0, 0.0);
	jau_rgbd.rgb = pow(input_texture.Sample(linear_clamp, hr_jitter_uv), 1.0 / 2.2);
	jau_rgbd.a = 1.0;// depth_buffer.Sample(linear_clamp, hr_jitter_uv);

	// Load history
	uint2 window_size_int = (uint2)window_size;
	uint index = hr_pixel_pos.y * window_size.x + hr_pixel_pos.x;
	float4 history = float4(0.0, 0.0, 0.0, 0.0);
	history.r = history_buffer[index * 8 + 4];
	history.g = history_buffer[index * 8 + 5];
	history.b = history_buffer[index * 8 + 6];
	history.a = history_buffer[index * 8 + 7];

	// Load alpha and depth_res
	float alpha = clamp(cnn_res[index * 2 + 0], 0.0, 1.0);
	float depth_res = clamp(cnn_res[index * 2 + 1], 0.0, 1.0);
	//float depth_res = cnn_res[index * 2 + 1];

	// Combine
	//alpha = 0.1; // Temp testing
	float4 output = history * (1.0 - alpha) + jau_rgbd * alpha;
	output.rgb = output.rgb;
	output.a *= depth_res;

	// Return
	return saturate(output);
}