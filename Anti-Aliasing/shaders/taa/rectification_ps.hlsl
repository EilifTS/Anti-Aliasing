Texture2D new_sample_tex : register(t0);
Texture2D history_tex : register(t1);
SamplerState linear_clamp : register(s0);

cbuffer Constants : register(b0)
{
	float2 window_size;
	float2 rec_window_size;
}

cbuffer TAABuffer : register(b1)
{
	matrix view_to_prev_frame_clip_matrix;
};

float3 rgbToYCoCg(float3 c)
{
	float3x3 m = float3x3(
		 0.25, 0.50,  0.25,
		 0.50, 0.00, -0.50,
		-0.25, 0.50, -0.25
		);
	return mul(c, m);
}

float3 YCoCgTorgb(float3 c)
{
	float3x3 m = float3x3(
		1.0,  1.0, -1.0,
		1.0,  0.0,  1.0,
		1.0, -1.0, -1.0
		);
	return mul(c, m);
}

float3 clipHistory(float3 start_color, float3 end_color, float3 min_color, float3 max_color)
{
	float3 ray_origin = start_color;
	float3 ray_dir = end_color - start_color;
	float3 rec_ray_dir = 1.0 / ray_dir;

	float3 min_intersect = (min_color - ray_origin) * rec_ray_dir;
	float3 max_intersect = (max_color - ray_origin) * rec_ray_dir;
	float3 enter_intersect = min(min_intersect, max_intersect);
	float x = max(enter_intersect.x, max(enter_intersect.y, enter_intersect.z));
	x = saturate(x);

	return lerp(start_color, end_color, x);
}

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

float4 PS(PSInput input) : SV_TARGET
{
	float4 new_sample = new_sample_tex.Sample(linear_clamp, input.uv);
	float4 history = history_tex.Sample(linear_clamp, input.uv);

	float3 ns_nw = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(-1.0, -1.0)).xyz;
	float3 ns_n = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(0.0, -1.0)).xyz;
	float3 ns_ne = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(1.0, -1.0)).xyz;
	float3 ns_w = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(-1.0, 0.0)).xyz;
	float3 ns_m = new_sample.xyz;
	float3 ns_e = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(1.0, 0.0)).xyz;
	float3 ns_sw = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(-1.0, 1.0)).xyz;
	float3 ns_s = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(0.0, 1.0)).xyz;
	float3 ns_se = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(1.0, 1.0)).xyz;

	ns_nw = rgbToYCoCg(ns_nw);
	ns_n = rgbToYCoCg(ns_n);
	ns_ne = rgbToYCoCg(ns_ne);
	ns_w = rgbToYCoCg(ns_w);
	ns_m = rgbToYCoCg(ns_m);
	ns_e = rgbToYCoCg(ns_e);
	ns_sw = rgbToYCoCg(ns_sw);
	ns_s = rgbToYCoCg(ns_s);
	ns_se = rgbToYCoCg(ns_se);

	float3 min_n = min(ns_nw, min(ns_n, ns_ne));
	float3 min_m = min(ns_w,  min(ns_m, ns_e));
	float3 min_s = min(ns_sw, min(ns_s, ns_se));
	float3 min_sample = min(min_n, min(min_m, min_s));

	float3 max_n = max(ns_nw, max(ns_n, ns_ne));
	float3 max_m = max(ns_w,  max(ns_m, ns_e));
	float3 max_s = max(ns_sw, max(ns_s, ns_se));
	float3 max_sample = max(max_n, max(max_m, max_s));

	float3 clipped_sample = clipHistory(rgbToYCoCg(history), 0.5 * (min_sample + max_sample), min_sample, max_sample);
	clipped_sample = YCoCgTorgb(clipped_sample);
	//float3 clamped_sample = clamp(rgbToYCoCg(history), min_sample, max_sample);
	//clamped_sample = YCoCgTorgb(clamped_sample);

	float alpha = 0.13;
	return lerp(float4(clipped_sample, 1.0), new_sample, alpha);
}