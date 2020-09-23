#include "../headers/camera.hlsli"

cbuffer CameraBuffer : register(b0)
{
	CameraBufferType camera;
}

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 view_position : TEXCOORD0;
	float2 uv : TEXCOORD1;
};

VSOutput VS(uint id : SV_VertexID)
{
	VSOutput output;
	output.uv = float2((id << 1) & 2, id & 2);
	output.position = float4(output.uv * float2(2.0, -2.0) - float2(1.0, -1.0), 0.0, 1.0);
	output.view_position = mul(float4(output.position.xy, 0.0, 1.0), camera.inv_projection_matrix);

	return output;
}