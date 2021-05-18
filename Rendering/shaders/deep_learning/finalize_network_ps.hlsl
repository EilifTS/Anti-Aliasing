Buffer<half> history_buffer : register(t0);
Buffer<half> cnn_res : register(t1);

Texture2D input_texture	: register(t2);
Texture2D<float> depth_buffer	: register(t3);
SamplerState linear_clamp : register(s0);

#ifndef UPSAMPLE_FACTOR
#define UPSAMPLE_FACTOR 4
#endif

#ifndef OPTIM
#define OPTIM 2
#endif

// Use this as index + c+r*r to get acctual index
uint get_pixel_shuffle_index(uint x, uint y, uint r, uint W, uint C)
{
	uint ym = y % r;
	uint xm = x % r;

	return (y - ym) * W * C + (x - xm) * r * C + r * ym + xm;
}

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

float3 manualBilinear(float2 uv)
{
	float2 position = uv * window_size / UPSAMPLE_FACTOR - 0.5;
	float2 center_position = floor(position);
	float2 f = position - center_position;
	int2 pos0 = int2(center_position);
	int2 pos1 = int2(center_position) + int2(1,1);

	float3 g00 = input_texture[int2(pos0.x, pos0.y)];
	float3 g01 = input_texture[int2(pos0.x, pos1.y)];
	float3 g10 = input_texture[int2(pos1.x, pos0.y)];
	float3 g11 = input_texture[int2(pos1.x, pos1.y)];

	float3 b0 = g00 * (1.0 - f.x) + g10 * f.x;
	float3 b1 = g01 * (1.0 - f.x) + g11 * f.x;
	return b0 * (1.0 - f.y) + b1 * f.y;
}

float4 PS(PSInput input) : SV_TARGET
{ 
	uint2 hr_pixel_pos = (uint2)input.position.xy; // Position in pixels of pixel in high res image
	uint2 lr_pixel_pos = hr_pixel_pos / UPSAMPLE_FACTOR; // Position of pixel in low res image
	float2 jitter_offset = current_jitter; // Pixel offset of jitter in low res image

	// JAU input
	float2 hr_jitter_pos = input.position.xy + (float2(0.5, 0.5) - jitter_offset) * UPSAMPLE_FACTOR;
	float2 hr_jitter_uv = hr_jitter_pos * rec_window_size;
	float4 jau_rgbd = float4(0.0, 0.0, 0.0, 0.0);
	jau_rgbd.rgb = input_texture.Sample(linear_clamp, hr_jitter_uv);
	//jau_rgbd.rgb = manualBilinear(hr_jitter_uv);
	jau_rgbd.a = 1.0;// depth_buffer.Sample(linear_clamp, hr_jitter_uv);

	// Load history
	uint2 window_size_int = (uint2)window_size;

#if OPTIM == 2
	uint index = get_pixel_shuffle_index(hr_pixel_pos.x, hr_pixel_pos.y, 4, window_size.x, 8);
	uint index0 = index + 4*4 * 4;
	uint index1 = index + 4*4 * 5;
	uint index2 = index + 4*4 * 6;
	uint index3 = index + 4*4 * 7;
#else
	uint index = hr_pixel_pos.y * window_size.x + hr_pixel_pos.x;
	uint index0 = index * 8 + 4;
	uint index1 = index * 8 + 5;
	uint index2 = index * 8 + 6;
	uint index3 = index * 8 + 7;
#endif

	float4 history = float4(0.0, 0.0, 0.0, 0.0);
	history.r = history_buffer[index0];
	history.g = history_buffer[index1];
	history.b = history_buffer[index2];
	history.a = history_buffer[index3];

	// Pixel shuffle
#if OPTIM == 0
	index0 = 2 * index + 0;
	index1 = 2 * index + 1;
#else
	index0 = get_pixel_shuffle_index(hr_pixel_pos.x, hr_pixel_pos.y, 4, window_size.x, 2);
	index1 = index0 + 4 * 4;
#endif

	// Load alpha and depth_res
	float alpha = clamp(cnn_res[index0], 0.0, 1.0);
	float depth_res = clamp(cnn_res[index1], 0.0, 1.0);
	//float depth_res = cnn_res[index * 2 + 1];

	// Combine
	//alpha = 0.1; // Temp testing
	float4 output = history * (1.0 - alpha) + jau_rgbd * alpha;
	output.a *= depth_res;

	//output.rgb = (1.0).xxx * (index % 16) / 16.0;

	// Return
	return saturate(output);
}