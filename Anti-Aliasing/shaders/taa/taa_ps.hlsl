
// Defines
#ifndef TAA_UPSAMPLE_FACTOR
#define TAA_UPSAMPLE_FACTOR 1
#endif

#if TAA_UPSAMPLE_FACTOR != 1
#define TAA_UPSAMPLE 1
#endif

#ifndef TAA_USE_CATMUL_ROM
#define TAA_USE_CATMUL_ROM 1
#endif

#ifndef TAA_USE_HISTORY_RECTIFICATION
#define TAA_USE_HISTORY_RECTIFICATION 1
#endif

#ifndef TAA_USE_YCOCG
#define TAA_USE_YCOCG 1
#endif

#ifndef TAA_USE_CLIPPING
#define TAA_USE_CLIPPING 1
#endif

Texture2D			new_sample_tex		: register(t0);
Texture2D			history_buffer		: register(t1);
Texture2D<float2>	motion_vectors		: register(t2);
Texture2D<float>	depth_buffer		: register(t3);
SamplerState		linear_clamp		: register(s0);

cbuffer constants : register(b0)
{
	float2 window_size;
	float2 rec_window_size;
}

cbuffer TAABuffer : register(b1)
{
	matrix clip_to_prev_frame_clip_matrix;
	float2 current_jitter;
};

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 clip_position : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

float calculateBeta(int2 pixel_pos, float2 j)
{
	// Calculate position of sample in history buffer
	float2 new_sample_pixel_pos_nw = float2(pixel_pos / TAA_UPSAMPLE_FACTOR);
	//float2 jittered_new_sample_pixel_pos = new_sample_pixel_pos_nw  + j;
	float2 jittered_new_sample_pixel_pos = new_sample_pixel_pos_nw  + j * float2(1.0, -1.0) + float2(0.0, 1.0);
	int2 new_sample_pixel_pos_in_history = int2(jittered_new_sample_pixel_pos * TAA_UPSAMPLE_FACTOR);

	// Use new sample if equal
	float beta = 1.0;
	if (new_sample_pixel_pos_in_history.x != pixel_pos.x) beta = 0.0;
	if (new_sample_pixel_pos_in_history.y != pixel_pos.y) beta = 0.0;
	return beta;
}

float3 catmulSample(float2 uv)
{
	return history_buffer.Sample(linear_clamp, uv).rgb;
}

float3 catmulRom(float2 uv)
{
	float2 ws_sub = window_size;
	float2 rws_sub = rec_window_size;
	float2 position = uv * ws_sub;
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
	float2 tc12 = rws_sub * (center_position + w2 / w12);
	float3 center_color = catmulSample(tc12);

	float2 tc0 = rws_sub * (center_position - 1.0);
	float2 tc3 = rws_sub * (center_position + 2.0);
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
	int2 new_sample_pos = pixel_pos / TAA_UPSAMPLE_FACTOR;
	float clip_depth = depth_buffer.Load(int3(new_sample_pos, 0));
	float4 clip_position = float4(float3(input.clip_position, clip_depth), 1.0f);

	// Dialate forground objects
	int2 offset_nw = int2(-2, -2);
	int2 offset_ne = int2( 2, -2);
	int2 offset_sw = int2(-2,  2);
	int2 offset_se = int2( 2,  2);
	float depth_nw = depth_buffer.Load(int3(new_sample_pos + offset_nw, 0));
	float depth_ne = depth_buffer.Load(int3(new_sample_pos + offset_ne, 0));
	float depth_sw = depth_buffer.Load(int3(new_sample_pos + offset_sw, 0));
	float depth_se = depth_buffer.Load(int3(new_sample_pos + offset_se, 0));

	int2 velocity_offset = offset_nw;
	float frontmost_depth = depth_nw;
	if (frontmost_depth > depth_ne)
	{
		velocity_offset = offset_ne;
		frontmost_depth = depth_ne;
	}
	if (frontmost_depth > depth_sw)
	{
		velocity_offset = offset_sw;
		frontmost_depth = depth_sw;
	}
	if (frontmost_depth > depth_se)
	{
		velocity_offset = offset_se;
		frontmost_depth = depth_se;
	}
	if (frontmost_depth > clip_depth)
	{
		velocity_offset = int2(0.0, 0.0);
	}
	else
	{
		clip_position.z = frontmost_depth;
	}

	// Calculate uv in history buffer
	float2 prev_frame_uv = float2(0.0, 0.0);

	// Static object pixel
	float4 prev_frame_pos = mul(clip_position, clip_to_prev_frame_clip_matrix);
	prev_frame_pos /= prev_frame_pos.w;
	prev_frame_uv = prev_frame_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	
	// Dynamic object pixel
	float2 motion_vector = motion_vectors.Load(int3(new_sample_pos + velocity_offset, 0));
	if (motion_vector.x != 0.0 || motion_vector.y != 0.0)
		prev_frame_uv = input.uv + motion_vector;

	// Sample history
	bool refresh_history = true;
	float4 history = float4(0.0, 0.0, 0.0, 0.0);
	if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0)
	{
		#if TAA_USE_CATMUL_ROM
			history = float4(catmulRom(prev_frame_uv), 1.0);
		#else
			history = history_buffer.Sample(linear_clamp, prev_frame_uv);
		#endif // TAA_USE_CATMUL_ROM
		
		refresh_history = false;
	}

	// History rectification
	
	float4 new_sample = new_sample_tex.Load(int3(new_sample_pos, 0));

#if TAA_USE_HISTORY_RECTIFICATION

	float3 ns_nw = new_sample_tex.Load(int3(new_sample_pos + int2(-1, -1), 0)).xyz;
	float3 ns_n =  new_sample_tex.Load(int3(new_sample_pos + int2( 0, -1), 0)).xyz;
	float3 ns_ne = new_sample_tex.Load(int3(new_sample_pos + int2( 1, -1), 0)).xyz;
	float3 ns_w =  new_sample_tex.Load(int3(new_sample_pos + int2(-1,  0), 0)).xyz;
	float3 ns_m =  new_sample.xyz;
	float3 ns_e =  new_sample_tex.Load(int3(new_sample_pos + int2( 1,  0), 0)).xyz;
	float3 ns_sw = new_sample_tex.Load(int3(new_sample_pos + int2(-1,  1), 0)).xyz;
	float3 ns_s =  new_sample_tex.Load(int3(new_sample_pos + int2( 0,  1), 0)).xyz;
	float3 ns_se = new_sample_tex.Load(int3(new_sample_pos + int2( 1,  1), 0)).xyz;

#if TAA_USE_YCOCG
	ns_nw = rgbToYCoCg(ns_nw);
	ns_n  = rgbToYCoCg(ns_n);
	ns_ne = rgbToYCoCg(ns_ne);
	ns_w  = rgbToYCoCg(ns_w);
	ns_m  = rgbToYCoCg(ns_m);
	ns_e  = rgbToYCoCg(ns_e);
	ns_sw = rgbToYCoCg(ns_sw);
	ns_s  = rgbToYCoCg(ns_s);
	ns_se = rgbToYCoCg(ns_se);
	history.rgb = rgbToYCoCg(history.rgb);
#endif // TAA_USE_YCOCG

	float3 min_n =		min(ns_nw, min(ns_n,  ns_ne));
	float3 min_m =		min(ns_w,  min(ns_m,  ns_e));
	float3 min_s =		min(ns_sw, min(ns_s,  ns_se));
	float3 min_sample = min(min_n, min(min_m, min_s));

	float3 max_n =		max(ns_nw, max(ns_n,  ns_ne));
	float3 max_m =		max(ns_w,  max(ns_m,  ns_e));
	float3 max_s =		max(ns_sw, max(ns_s,  ns_se));
	float3 max_sample = max(max_n, max(max_m, max_s));

#if TAA_USE_CLIPPING
	float3 clipped_sample = clipHistory(history.rgb, 0.5 * (min_sample + max_sample), min_sample, max_sample);
	history = float4(clipped_sample, 1.0);
#else
	float3 clamped_sample = clamp(history.rgb, min_sample, max_sample);
	history = float4(clamped_sample, 1.0);
#endif // TAA_USE_CLIPPING

#if TAA_USE_YCOCG
	history.rgb = YCoCgTorgb(history.rgb);
#endif // TAA_USE_YCOCG

#endif // TAA_USE_HISTORY_RECTIFICATION

	// Calculate beta for upsampling
	float beta = 1.0; 
#if TAA_UPSAMPLE
	//if (int(floor(input.uv.x * window_size.x / 2)) % 2 == 0)
	//	beta = 0;
	beta = calculateBeta(pixel_pos, current_jitter);
	//beta = 1.0;
#endif

	float velocity = length((prev_frame_uv - input.uv) * window_size);
	float velocity_f = saturate(velocity / 20);
	float alpha = lerp(0.2, 0.2, velocity_f);

	if (refresh_history) alpha = 1.0;

	//return float4(beta.xxx, 1.0);
	return lerp(history, new_sample, alpha * beta);
}