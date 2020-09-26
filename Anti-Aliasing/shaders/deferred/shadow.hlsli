
float calculateShadow(Texture2D shadowmap, SamplerComparisonState samp, float3 view_pos, matrix shadow_matrix)
{
	float3 shadow_map_pos = mul(float4(view_pos, 1.0), shadow_matrix).xyz;
	float2 shadow_map_uv = shadow_map_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	return shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z);
}

float calculateShadowBad(Texture2D shadowmap, SamplerState samp, float3 view_pos, matrix shadow_matrix)
{
	float3 shadow_map_pos = mul(float4(view_pos, 1.0), shadow_matrix).xyz;
	float2 shadow_map_uv = shadow_map_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	float s = shadowmap.Sample(samp, shadow_map_uv);
	return s > shadow_map_pos.z ? 1.0f : 0.0f;
}

float calculateShadowSmooth(Texture2D shadowmap, SamplerComparisonState samp, float3 view_pos, matrix shadow_matrix)
{
	float3 shadow_map_pos = mul(float4(view_pos, 1.0), shadow_matrix).xyz;
	float2 shadow_map_uv = shadow_map_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	float shadow_factor = 0.0;
	
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0,0));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1,0));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1,0));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0,1));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0,-1));

	return shadow_factor * (1.0 / 5.0);
}

float calculateShadowSmooth2(Texture2D shadowmap, SamplerComparisonState samp, float3 view_pos, matrix shadow_matrix)
{
	float3 shadow_map_pos = mul(float4(view_pos, 1.0), shadow_matrix).xyz;
	float2 shadow_map_uv = shadow_map_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	float shadow_factor = 0.0;

	if (shadow_map_pos.z > 1.0 || 
		shadow_map_pos.z < 0.0 ||
		shadow_map_uv.x < 0.0 ||
		shadow_map_uv.x > 1.0 ||
		shadow_map_uv.y < 0.0 ||
		shadow_map_uv.y > 1.0
		)
		return 0.0;

	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, 0)) * 2;
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1, 0));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1, 0));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, 1));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, -1));

	return shadow_factor * (1.0 / 6.0);
}

float calculateShadowSmooth3(Texture2D shadowmap, SamplerComparisonState samp, float3 view_pos, matrix shadow_matrix)
{
	float3 shadow_map_pos = mul(float4(view_pos, 1.0), shadow_matrix).xyz;
	float2 shadow_map_uv = shadow_map_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	float shadow_factor = 0.0;

	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1, 1));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, 1));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1, 1));

	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1, 0));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, 0));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1, 0));

	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1, -1));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, -1));
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1, -1));

	return shadow_factor * (1.0 / 9.0);
}

float calculateShadowSmooth4(Texture2D shadowmap, SamplerComparisonState samp, float3 view_pos, matrix shadow_matrix)
{
	float3 shadow_map_pos = mul(float4(view_pos, 1.0), shadow_matrix).xyz;
	float2 shadow_map_uv = shadow_map_pos.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
	float shadow_factor = 0.0;

	if (shadow_map_pos.z > 1.0 ||
		shadow_map_pos.z < 0.0 ||
		shadow_map_uv.x < 0.0 ||
		shadow_map_uv.x > 1.0 ||
		shadow_map_uv.y < 0.0 ||
		shadow_map_uv.y > 1.0
		)
		return 0.0;

	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1, 1)) * 0.25f;
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, 1)) * 0.5f;
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1, 1)) * 0.25f;

	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1, 0)) * 0.5f;
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, 0)) * 1.0f;
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1, 0)) * 0.5f;

	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(-1, -1)) * 0.25f;
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(0, -1)) * 0.5f;
	shadow_factor += shadowmap.SampleCmpLevelZero(samp, shadow_map_uv, shadow_map_pos.z, int2(1, -1)) * 0.25f;

	return shadow_factor * (1.0 / 4.0);
}