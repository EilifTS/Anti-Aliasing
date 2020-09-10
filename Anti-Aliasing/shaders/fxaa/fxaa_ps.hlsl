#include "fxaa_header.hlsli"

Texture2D tex : register(t0);
SamplerState linear_clamp : register(s0);
cbuffer fxaa_buffer : register(b0)
{
	float2 reciprocal_texture_dims;
}

// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

// Luma functions ranging from highest quality to lowest
float luma(float3 color)
{
	return dot(color, float3(0.299, 0.587, 0.114));
}
float luma2(float3 color)
{
	return color.g* (0.587 / 0.299) + color.r;
}
float luma3(float3 color)
{
	return color.g;
}

float3 sampleAt(float2 pos)
{
	return tex.Sample(linear_clamp, pos).xyz;
}
float3 sampleAtOffset(float2 pos, float2 offset)
{
	return tex.Sample(linear_clamp, pos + offset * reciprocal_texture_dims).xyz;
}

float4 PS(PSInput input) : SV_TARGET
{
	float3 rgb_m = sampleAtOffset(input.uv, float2(0.0, 0.0));

#ifdef DEBUG_NO_FXAA
	return float4(rgb_m, 1.0);
#endif

	float3 rgb_n = sampleAtOffset(input.uv, float2(0.0, -1.0));
	float3 rgb_w = sampleAtOffset(input.uv, float2(-1.0, 0.0));
	float3 rgb_e = sampleAtOffset(input.uv, float2(1.0, 0.0));
	float3 rgb_s = sampleAtOffset(input.uv, float2(0.0, 1.0));
	float luma_n = luma(rgb_n);
	float luma_w = luma(rgb_w);
	float luma_m = luma(rgb_m);
	float luma_e = luma(rgb_e);
	float luma_s = luma(rgb_s);

	// Edge detection
	float luma_min = min(luma_n, min(min(luma_w, luma_m), min(luma_e, luma_s)));
	float luma_max = max(luma_n, max(max(luma_w, luma_m), max(luma_e, luma_s)));
	float luma_range = luma_max - luma_min;

	if (luma_range < max(EDGE_THRESHOLD_MIN, luma_max * EDGE_THRESHOLD))
		return float4(rgb_m, 1.0);

#ifdef DEBUG_SHOW_EDGES
	return float4(1.0, 0.0, 0.0, 1.0);
#endif

	float3 rgb_nw = sampleAtOffset(input.uv, float2(-1.0, -1.0));
	float3 rgb_ne = sampleAtOffset(input.uv, float2(1.0, -1.0));
	float3 rgb_sw = sampleAtOffset(input.uv, float2(-1.0, 1.0));
	float3 rgb_se = sampleAtOffset(input.uv, float2(1.0, 1.0));
	float luma_nw = luma(rgb_nw);
	float luma_ne = luma(rgb_ne);
	float luma_sw = luma(rgb_sw);
	float luma_se = luma(rgb_se);

	// Edge direction detection
	float edge_vert =
		abs(0.25 * luma_nw - 0.50 * luma_n + 0.25 * luma_ne) +
		abs(0.50 * luma_w  - 1.00 * luma_m + 0.50 * luma_e ) +
		abs(0.25 * luma_sw - 0.50 * luma_s + 0.25 * luma_se);
	float edge_horz =
		abs(0.25 * luma_nw - 0.50 * luma_w + 0.25 * luma_sw) +
		abs(0.50 * luma_n  - 1.00 * luma_m + 0.50 * luma_s ) +
		abs(0.25 * luma_ne - 0.50 * luma_e + 0.25 * luma_se);

	bool is_horz = edge_horz >= edge_vert;

#ifdef DEBUG_SHOW_EDGE_DIRECTION
	return is_horz ? float4(0.0, 1.0, 0.0, 1.0) : float4(0.0, 0.0, 1.0, 1.0);
#endif

	// Find edge side
	float luma1 = is_horz ? luma_n : luma_w;
	float luma2 = is_horz ? luma_s : luma_e;

	float grad1 = luma1 - luma_m;
	float grad2 = luma2 - luma_m;

	bool is1 = abs(grad1) >= abs(grad2);
	float gradient_scaled = 0.25 * max(abs(grad1), abs(grad2));

#ifdef DEBUG_SHOW_EDGE_SIDE
	return is1 ? float4(0.0, 1.0, 1.0, 1.0) : float4(1.0, 0.0, 1.0, 1.0);
#endif

	// Move to center of edge
	float2 step_dir = (is_horz ? float2(0.0, 1.0) : float2(1.0, 0.0)) * reciprocal_texture_dims;
	float luma_local_avg = 0.0;

	if (is1)
	{
		step_dir = -step_dir;
		luma_local_avg = 0.5 * (luma_m + luma1);
	}
	else
	{
		luma_local_avg = 0.5 * (luma_m + luma2);
	}

	float2 current_uv = input.uv + 0.5 * step_dir;

	// Edge traversal
	float2 offset = (is_horz ? float2(1.0, 0.0) : float2(0.0, 1.0)) * reciprocal_texture_dims;
	float2 uv1 = current_uv + offset;
	float2 uv2 = current_uv - offset;
	bool reached1 = false;
	bool reached2 = false;
	float luma_end1 = 0.0;
	float luma_end2 = 0.0;
	for (int i = 0; i < TAVERSAL_MAX_STEPS; i++)
	{
		if (!reached1)
		{
			luma_end1 = luma(sampleAt(uv1));
			luma_end1 = luma_end1 - luma_local_avg;
		}
		if (!reached2)
		{
			luma_end2 = luma(sampleAt(uv2));
			luma_end2 = luma_end2 - luma_local_avg;
		}

		reached1 = abs(luma_end1) >= gradient_scaled;
		reached2 = abs(luma_end2) >= gradient_scaled;

		if (!reached1)
			uv1 += offset;
		if (!reached2)
			uv2 -= offset;
		if (reached1 && reached2)
			break;
	}

	// Estimate offset
	float distance1 = is_horz ? (uv1.x - current_uv.x) : (uv1.y - current_uv.y);
	float distance2 = is_horz ? (current_uv.x - uv2.x) : (current_uv.y - uv2.y);

	bool is1_closest = distance1 < distance2;
	float shortest_dist = min(distance1, distance2);
	float total_dist = distance1 + distance2;
	float pixel_offset = -shortest_dist / total_dist + 0.5;

#ifdef DEBUG_SHOW_CLOSEST_END
	return is1_closest ? float4(1.0, 1.0, 0.0, 1.0) : float4(1.0, 0.0, 1.0, 1.0);
#endif

	// Check if distance is not too far (might not be needed with stepsize 1?)
	bool luma_m_smaller = luma_m < luma_local_avg;
	bool correct_variation_at_end = ((is1_closest ? luma_end1 : luma_end2) < 0.0) != luma_m_smaller;
	pixel_offset = correct_variation_at_end ? pixel_offset : 0.0;

	// Subpixel anti-aliasing
	float luma_average = (1.0 / 12.0) * (2.0 * (luma_n + luma_w + luma_e + luma_s) + luma_nw + luma_ne + luma_sw + luma_se);
	float sub_pixel_offset = saturate(abs(luma_average - luma_m) / luma_range);
	sub_pixel_offset = (3.0 - 2.0 * sub_pixel_offset) * sub_pixel_offset * sub_pixel_offset;
	sub_pixel_offset = sub_pixel_offset * sub_pixel_offset * SUBPIXEL_QUALITY;

	// Combine edge antialiasing with subpixel antialiasing
#ifndef DEBUG_NO_SUBPIXEL_ANTIALIASING
	pixel_offset = max(sub_pixel_offset, pixel_offset);
#endif

	// Get final color
	float3 color = sampleAt(input.uv + step_dir * pixel_offset);
	return float4(color, 1.0);
}