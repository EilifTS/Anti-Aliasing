
// Defines
#ifndef TAA_UPSAMPLE_FACTOR
#define TAA_UPSAMPLE_FACTOR 1
#endif

#ifndef TAA_UPSAMPLE
#define TAA_UPSAMPLE 0
#endif


#ifndef TAA_USE_RASTERIZER
#define TAA_USE_RASTERIZER 1
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

Texture2D			new_sample_buffer	: register(t0);
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

// Only used for ray tracing
cbuffer JitterBuffer : register(b2)
{
	int jitter_count;
	int sample_count;
	int jitter_index;
	int placeholder;
	float4 jitters[128];
}

int cash(uint x, uint y)
{
	int h = 0 + x * 374761393 + y * 668265263;
	h = (h ^ (h >> 13)) * 1274126177;
	return h ^ (h >> 16);
}

// Per pixel input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 clip_position : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

static const int2 index3x3[9] =
{
	int2(-1, -1), int2(0, -1), int2(1, -1),
	int2(-1,  0), int2(0,  0), int2(1,  0),
	int2(-1,  1), int2(0,  1), int2(1,  1)
};

// Functions for converting positions between high resolution, low resolution and uv
float2 HR2UV(float2 hr_pos)
{
	return hr_pos * rec_window_size;
}

float2 LR2UV(float2 lr_pos)
{
	return lr_pos * rec_window_size * TAA_UPSAMPLE_FACTOR;
}

float2 UV2HR(float2 uv)
{
	return uv * window_size;
}

float2 UV2LR(float2 uv)
{
	return uv * window_size / TAA_UPSAMPLE_FACTOR;
}

// Returns the weight of of a pixel at distance dp
float computePixelWeight(float2 dp, float inv_scale_factor)
{
	// Formula is 1 - 1.9 * x^2 + 0.9 * x^4
	// It is an approximation of e ^ (- x^2 / (2 * s^2))
	// With s = 0.47
	float u2 = TAA_UPSAMPLE_FACTOR * TAA_UPSAMPLE_FACTOR;
	u2 *= inv_scale_factor * inv_scale_factor;

	float x2 = u2 * dot(dp, dp);
	float r = (0.905 * x2 - 1.9) * x2 + 1.0;
	if (x2 > 1.0) r = 0.000000001;
	return r * u2;
}

// Returns 1 if jitter is in current pixel in upscaled resolution
float calculateBeta(int2 pixel_pos, float2 j)
{
	// Calculate position of sample in history buffer
	float2 new_sample_pixel_pos_nw = float2(pixel_pos / TAA_UPSAMPLE_FACTOR);
	float2 jittered_new_sample_pixel_pos = new_sample_pixel_pos_nw  + j;
	int2 new_sample_pixel_pos_in_history = int2(jittered_new_sample_pixel_pos * TAA_UPSAMPLE_FACTOR);

	// Use new sample if equal
	float beta = 1.0;
	if (new_sample_pixel_pos_in_history.x != pixel_pos.x) beta = 0.0;
	if (new_sample_pixel_pos_in_history.y != pixel_pos.y) beta = 0.0;
	return beta;
}

float2 calculatePreviousFrameUV(int2 new_sample_pos, float2 uv, float4 clip_position)
{
	// Dialate forground objects
	const int dialation_offset = 1;
	int2 offset_nw = int2(-dialation_offset, -dialation_offset);
	int2 offset_ne = int2(dialation_offset, -dialation_offset);
	int2 offset_sw = int2(-dialation_offset, dialation_offset);
	int2 offset_se = int2(dialation_offset, dialation_offset);
	float depth_nw = depth_buffer.Sample(linear_clamp, uv, offset_nw);
	float depth_ne = depth_buffer.Sample(linear_clamp, uv, offset_ne);
	float depth_sw = depth_buffer.Sample(linear_clamp, uv, offset_sw);
	float depth_se = depth_buffer.Sample(linear_clamp, uv, offset_se);

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
	if (frontmost_depth > clip_position.z)
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
		prev_frame_uv = uv + motion_vector;

	return prev_frame_uv;
}

float3 catmullSample(float2 uv)
{
	return history_buffer.Sample(linear_clamp, uv).rgb;
}

float3 catmullRom(float2 uv)
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
	float3 center_color = catmullSample(tc12);

	float2 tc0 = rec_window_size * (center_position - 1.0);
	float2 tc3 = rec_window_size * (center_position + 2.0);
	float4 color =
		float4(catmullSample(float2(tc12.x, tc0.y)), 1.0) * (w12.x * w0.y) +
		float4(catmullSample(float2(tc0.x, tc12.y)), 1.0) * (w0.x * w12.y) +
		float4(center_color, 1.0) * (w12.x * w12.y) +
		float4(catmullSample(float2(tc3.x, tc12.y)), 1.0) * (w3.x * w12.y) +
		float4(catmullSample(float2(tc12.x, tc3.y)), 1.0) * (w12.x * w3.y);

	return color.rgb / color.a;
}

float3 sampleHistoryColor(float2 prev_frame_uv, inout bool refresh_history)
{
	float3 history = float3(0.0, 0.0, 0.0);
	if (prev_frame_uv.x > 0.0 && prev_frame_uv.x <= 1.0 && prev_frame_uv.y > 0.0 && prev_frame_uv.y <= 1.0)
	{
#if TAA_USE_CATMUL_ROM
		history = catmullRom(prev_frame_uv);
#else
		history = history_buffer.Sample(linear_clamp, prev_frame_uv).xyz;
#endif // TAA_USE_CATMUL_ROM

		refresh_history = false;
	}
	return history;
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

float3 rectifyHistory(float3 history, float3 new_samples[9])
{
	// Transform into YCoCg
	#if TAA_USE_YCOCG
		[unroll]
		for (int i = 0; i < 9; i++)
			new_samples[i] = rgbToYCoCg(new_samples[i]);
		history = rgbToYCoCg(history);
	#endif // TAA_USE_YCOCG

	// Calculate AABB
	float3 min_n = min(new_samples[0], min(new_samples[1], new_samples[2]));
	float3 min_m = min(new_samples[3], min(new_samples[4], new_samples[5]));
	float3 min_s = min(new_samples[6], min(new_samples[7], new_samples[8]));
	float3 min_sample = min(min_n, min(min_m, min_s));

	float3 max_n = max(new_samples[0], max(new_samples[1], new_samples[2]));
	float3 max_m = max(new_samples[3], max(new_samples[4], new_samples[5]));
	float3 max_s = max(new_samples[6], max(new_samples[7], new_samples[8]));
	float3 max_sample = max(max_n, max(max_m, max_s));

	// Do clipping or clamping
	#if TAA_USE_CLIPPING
		history = clipHistory(history.rgb, 0.5 * (min_sample + max_sample), min_sample, max_sample);
	#else
		history = clamp(history.rgb, min_sample, max_sample);
	#endif // TAA_USE_CLIPPING

	// Transform back to RGB
	#if TAA_USE_YCOCG
		history = YCoCgTorgb(history);
	#endif // TAA_USE_YCOCG

	return history;
}

#define MIN_ALPHA 0.05
#define MAX_ALPHA 0.2
float calculateAlpha(float2 uv, float2 prev_frame_uv, bool refresh_history)
{
	// Calculate velocity
	float velocity = length((prev_frame_uv - uv) * window_size);
	float velocity_f = saturate(velocity / 10);

	// Calculate alpha
	float alpha = lerp(MIN_ALPHA, MAX_ALPHA, velocity_f);
	if (refresh_history) alpha = 1.0;

	return alpha;
}

float4 PS(PSInput input) : SV_TARGET
{
	// Some position calculations
	int2 pixel_pos = (int2)input.position.xy; // Position in pixels of current pixel in high res image
	int2 new_sample_pos = pixel_pos / TAA_UPSAMPLE_FACTOR; // Position of pixel in low res image

#if TAA_USE_RASTERIZER
	float2 jitter_offset = current_jitter; // Pixel offset of jitter in low res image
#else
	int prev_j_index = (jitter_index - 1) % sample_count;
	int random_jitter_index = (cash(new_sample_pos.x, new_sample_pos.y) + prev_j_index) % jitter_count;
	float2 jitter_offset = jitters[random_jitter_index].xy; // Pixel offset of jitter in low res image
#endif

	float clip_depth = depth_buffer.Sample(linear_clamp, input.uv);
	float4 clip_position = float4(float3(input.clip_position, clip_depth), 1.0f);

	float2 prev_frame_uv = calculatePreviousFrameUV(new_sample_pos, input.uv, clip_position);

	// Sample history
	bool refresh_history = true;
	float3 history = sampleHistoryColor(prev_frame_uv, refresh_history);

	// Get samples around current pixel
	float3 new_samples[9];
	[unroll]
	for (int i = 0; i < 9; i++)
		new_samples[i] = new_sample_buffer.Load(int3(new_sample_pos + index3x3[i], 0)).xyz;

	// Calculate the new sample
#if TAA_UPSAMPLE
	float pixel_weights[9];
	float norm_weight_factor = 0.0;
	float3 new_sample = float3(0.0, 0.0, 0.0);
	float biggest_weight = 0.0;
	[unroll]
	for (i = 0; i < 9; i++)
	{
		float2 pixel_position = float2(new_sample_pos + index3x3[i]) + jitter_offset; // Position of pixel i in LR image
		float2 dp = pixel_position - UV2LR(input.uv);
		pixel_weights[i] = computePixelWeight(dp, 1.0);
		norm_weight_factor += pixel_weights[i];
		new_sample += new_samples[i] * pixel_weights[i];
		biggest_weight = max(biggest_weight, pixel_weights[i]);
	}

	// Center weight is always larger than the others
	//float biggest_weight = pixel_weights[4];

	float inv_norm_weight_factor = 1.0 / norm_weight_factor;
	new_sample *= inv_norm_weight_factor;

#else
	float3 new_sample = new_samples[4];
#endif // TAA_UPSAMPLE

	// History rectification
#if TAA_USE_HISTORY_RECTIFICATION
	history = rectifyHistory(history, new_samples);
#endif // TAA_USE_HISTORY_RECTIFICATION

	// Calculate beta for upsampling
	float beta = 1.0;
#if TAA_UPSAMPLE
	beta = biggest_weight;
#endif

	float alpha = calculateAlpha(input.uv, prev_frame_uv, refresh_history);

	return float4(lerp(history, new_sample, alpha * beta), 1.0);
}