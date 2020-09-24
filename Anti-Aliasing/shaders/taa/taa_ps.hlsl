
Texture2D			new_sample_tex		: register(t0);
Texture2D			history_buffer		: register(t1);
Texture2D<float2>	motion_vectors		: register(t2);
Texture2D<float>	depth_buffer		: register(t3);
Texture2D<int2>		stencil_buffer		: register(t4);
SamplerState		linear_clamp		: register(s0);

cbuffer constants : register(b0)
{
	float2 window_size;
	float2 rec_window_size;
}

cbuffer TAABuffer : register(b1)
{
	matrix clip_to_prev_frame_clip_matrix;
};

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 clip_position : TEXCOORD0;
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

float3 rgbToYCoCg(float3 c)
{
	float3x3 m = float3x3(
		0.25, 0.50, 0.25,
		0.50, 0.00, -0.50,
		-0.25, 0.50, -0.25
		);
	return mul(c, m);
}

float3 YCoCgTorgb(float3 c)
{
	float3x3 m = float3x3(
		1.0, 1.0, -1.0,
		1.0, 0.0, 1.0,
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

float4 PS(PSInput input) : SV_TARGET
{
	int2 pixel_pos = (int2)input.position.xy;
	float clip_depth = depth_buffer.Sample(linear_clamp, input.uv);
	int stencil = stencil_buffer.Load(int3(input.position.xy, 0)).y;
	float4 clip_position = float4(float3(input.clip_position, clip_depth), 1.0f);


	// Calculate uv in history buffer
	float2 prev_frame_uv = float2(0.0, 0.0);
	if (stencil == 0) // Static object pixel
	{
		float4 prev_frame_pos = mul(clip_position, clip_to_prev_frame_clip_matrix);
		prev_frame_pos /= prev_frame_pos.w;
		prev_frame_uv = prev_frame_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	}
	else // Dynamic object pixel
	{
		prev_frame_uv = input.uv + motion_vectors.Sample(linear_clamp, input.uv);
	}

	// Sample history
	float4 history = float4(0.0, 0.0, 0.0, 0.0);
	if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0)
		history = float4(catmulRom(prev_frame_uv), 1.0);
		//history = history_buffer.Sample(linear_clamp, prev_frame_uv);


	// History rectification
	float3 new_sample = new_sample_tex.Sample(linear_clamp, input.uv).xyz;
	float3 ns_nw = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(-1.0, -1.0)).xyz;
	float3 ns_n =  new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2( 0.0, -1.0)).xyz;
	float3 ns_ne = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2( 1.0, -1.0)).xyz;
	float3 ns_w =  new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(-1.0,  0.0)).xyz;
	float3 ns_m =  new_sample;
	float3 ns_e =  new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2( 1.0,  0.0)).xyz;
	float3 ns_sw = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2(-1.0,  1.0)).xyz;
	float3 ns_s =  new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2( 0.0,  1.0)).xyz;
	float3 ns_se = new_sample_tex.Sample(linear_clamp, input.uv + rec_window_size * float2( 1.0,  1.0)).xyz;

	ns_nw = rgbToYCoCg(ns_nw);
	ns_n  = rgbToYCoCg(ns_n);
	ns_ne = rgbToYCoCg(ns_ne);
	ns_w  = rgbToYCoCg(ns_w);
	ns_m  = rgbToYCoCg(ns_m);
	ns_e  = rgbToYCoCg(ns_e);
	ns_sw = rgbToYCoCg(ns_sw);
	ns_s  = rgbToYCoCg(ns_s);
	ns_se = rgbToYCoCg(ns_se);

	float3 min_n =		min(ns_nw, min(ns_n,  ns_ne));
	float3 min_m =		min(ns_w,  min(ns_m,  ns_e));
	float3 min_s =		min(ns_sw, min(ns_s,  ns_se));
	float3 min_sample = min(min_n, min(min_m, min_s));

	float3 max_n =		max(ns_nw, max(ns_n,  ns_ne));
	float3 max_m =		max(ns_w,  max(ns_m,  ns_e));
	float3 max_s =		max(ns_sw, max(ns_s,  ns_se));
	float3 max_sample = max(max_n, max(max_m, max_s));

	float3 clipped_sample = clipHistory(rgbToYCoCg(history.rgb), 0.5 * (min_sample + max_sample), min_sample, max_sample);
	clipped_sample = YCoCgTorgb(clipped_sample);
	//float3 clamped_sample = clamp(rgbToYCoCg(history), min_sample, max_sample);
	//clamped_sample = YCoCgTorgb(clamped_sample);

	float alpha = 0.13;
	float4 color = (0.0).xxxx;
	color = float4(lerp(clipped_sample, new_sample, alpha), 1.0);
	return color;
}