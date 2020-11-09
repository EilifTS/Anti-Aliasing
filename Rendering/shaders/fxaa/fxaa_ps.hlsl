#include "fxaa_header.hlsli"

// Algorithm input
Texture2D tex : register(t0);
SamplerState linear_clamp : register(s0);
cbuffer fxaa_buffer : register(b0)
{
	float2 reciprocal_texture_dims;
}

// Per pixel input structure
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
float lumaAt(float2 pos)
{
	return luma(tex.Load(int3(pos, 0)).xyz);
}

struct FXAACommon
{
	// Rgb of center pixel
	float3 rgb_m;

	// Luma of neighborhood
	float nw, n, ne;
	float w, m, e;
	float sw, s, se;

	float luma_range;

	// Luma for the pixel on the opposite side of the edge
	float opposite_luma;

	// Gradient over the edge
	float grad;

};

void initializeFXAACommon(inout FXAACommon common)
{
	common.rgb_m = float3(0.0, 0.0, 0.0);
	common.nw = 0.0; common.n = 0.0; common.ne = 0.0;
	common.w = 0.0; common.m = 0.0; common.e = 0.0;
	common.sw = 0.0; common.s = 0.0; common.se = 0.0;
	common.luma_range = 0.0;
	common.opposite_luma = 0.0;
	common.grad = 0.0;
}

bool detectEdge(inout FXAACommon common, int2 pixel_index)
{
	common.rgb_m = tex.Load(int3(pixel_index, 0)).xyz;

	common.n = lumaAt(pixel_index + int2(0, -1));
	common.w = lumaAt(pixel_index + int2(-1, 0));
	common.m = luma(common.rgb_m);
	common.e = lumaAt(pixel_index + int2(1, 0));
	common.s = lumaAt(pixel_index + int2(0, 1));

	// Edge detection
	float luma_min = min(common.n, min(min(common.w, common.m), min(common.e, common.s)));
	float luma_max = max(common.n, max(max(common.w, common.m), max(common.e, common.s)));
	common.luma_range = luma_max - luma_min;

	return common.luma_range >= max(EDGE_THRESHOLD_MIN, luma_max* EDGE_THRESHOLD);
}

bool findEdgeDirection(inout FXAACommon common, int2 pixel_index)
{
	common.nw = lumaAt(pixel_index + int2(-1, -1));
	common.ne = lumaAt(pixel_index + int2(1, -1));
	common.sw = lumaAt(pixel_index + int2(-1, 1));
	common.se = lumaAt(pixel_index + int2(1, 1));

	// Edge direction detection
	float edge_vert =
		abs(0.25 * common.nw - 0.50 * common.n + 0.25 * common.ne) +
		abs(0.50 * common.w  - 1.00 * common.m + 0.50 * common.e) +
		abs(0.25 * common.sw - 0.50 * common.s + 0.25 * common.se);
	float edge_horz =
		abs(0.25 * common.nw - 0.50 * common.w + 0.25 * common.sw) +
		abs(0.50 * common.n  - 1.00 * common.m + 0.50 * common.s) +
		abs(0.25 * common.ne - 0.50 * common.e + 0.25 * common.se);

	return edge_horz >= edge_vert;
}

bool findSide(inout FXAACommon common, bool is_horizontal)
{
	// Find edge side
	float luma1 = is_horizontal ? common.n : common.w;
	float luma2 = is_horizontal ? common.s : common.e;

	float grad1 = luma1 - common.m;
	float grad2 = luma2 - common.m;

	bool is1 = abs(grad1) >= abs(grad2);
	common.opposite_luma = is1 ? luma1 : luma2;
	common.grad = is1 ? grad1 : grad2;

	return is1;
}

float traverseEdge(FXAACommon common, float2 uv, bool is_horizontal, bool is_side1, inout float2 step_dir, inout bool is_1_closest)
{
	// Move to center of edge
	step_dir = (is_horizontal ? float2(0.0, 1.0) : float2(1.0, 0.0)) * reciprocal_texture_dims;
	float luma_local_avg = 0.5 * (common.m + common.opposite_luma);

	if (is_side1)
	{
		step_dir = -step_dir;
	}

	float2 current_uv = uv + 0.5 * step_dir;
	float gradient_scaled = 0.25 * abs(common.grad);

	// Edge traversal
	float2 offset = (is_horizontal ? float2(1.0, 0.0) : float2(0.0, 1.0)) * reciprocal_texture_dims;
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
	float distance1 = is_horizontal ? (uv1.x - current_uv.x) : (uv1.y - current_uv.y);
	float distance2 = is_horizontal ? (current_uv.x - uv2.x) : (current_uv.y - uv2.y);

	is_1_closest = distance1 < distance2;
	float shortest_dist = min(distance1, distance2);
	float total_dist = distance1 + distance2;
	float pixel_offset = -shortest_dist / total_dist + 0.5;


	// Check if distance is not too far (might not be needed with stepsize 1?)
	bool luma_m_smaller = common.m < luma_local_avg;
	bool correct_variation_at_end = ((is_1_closest ? luma_end1 : luma_end2) < 0.0) != luma_m_smaller;
	pixel_offset = correct_variation_at_end ? pixel_offset : 0.0;

	return pixel_offset;
}

float calculateSubpixelOffset(FXAACommon common)
{
	// Subpixel anti-aliasing
	float luma_average = (1.0 / 12.0) * 
		(2.0 * (common.n + common.w + common.e + common.s) +
			common.nw + common.ne + common.sw + common.se);
	float sub_pixel_offset = saturate(abs(luma_average - common.m) / common.luma_range);
	sub_pixel_offset = (3.0 - 2.0 * sub_pixel_offset) * sub_pixel_offset * sub_pixel_offset;
	return sub_pixel_offset * sub_pixel_offset * SUBPIXEL_QUALITY;
}

float4 PS(PSInput input) : SV_TARGET
{
	int2 pixel_index = int2(input.position.xy);
	FXAACommon common;
	initializeFXAACommon(common);

	bool is_on_edge = detectEdge(common, pixel_index);

	if(!is_on_edge) return float4(common.rgb_m, 1.0);

#ifdef DEBUG_SHOW_EDGES
	return float4(1.0, 0.0, 0.0, 1.0);
#endif

	bool is_horizontal = findEdgeDirection(common, pixel_index);

#ifdef DEBUG_SHOW_EDGE_DIRECTION
	return is_horizontal ? float4(0.0, 1.0, 0.0, 1.0) : float4(0.0, 0.0, 1.0, 1.0);
#endif

	bool is_side1 = findSide(common, is_horizontal);

#ifdef DEBUG_SHOW_EDGE_SIDE
	return is_side1 ? float4(0.0, 1.0, 1.0, 1.0) : float4(1.0, 0.0, 1.0, 1.0);
#endif
	bool is_1_closest = false;
	float2 step_dir;
	float pixel_offset = traverseEdge(common, input.uv, is_horizontal, is_side1, step_dir, is_1_closest);

#ifdef DEBUG_SHOW_CLOSEST_END
	return is_1_closest ? float4(1.0, 1.0, 0.0, 1.0) : float4(1.0, 0.0, 1.0, 1.0);
#endif

	float sub_pixel_offset = calculateSubpixelOffset(common);

	// Combine edge antialiasing with subpixel antialiasing
#ifndef DEBUG_NO_SUBPIXEL_ANTIALIASING
	pixel_offset = max(sub_pixel_offset, pixel_offset);
#endif

	// Get final color
	float3 color = sampleAt(input.uv + step_dir * pixel_offset);
	return float4(color, 1.0);
}