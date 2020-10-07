#include "../headers/camera.hlsli"
#include "../headers/model.hlsli"

cbuffer CameraBuffer : register(b0)
{
	CameraBufferType camera;
}
cbuffer ModelBuffer : register(b1)
{
	ModelBufferType model;
}

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL0;
	float4 tangent : NORMAL1;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};

VSOutput VS(VSInput input)
{
	VSOutput output;
	output.position = mul(float4(input.position, 1.0), model.world_matrix);
	output.position = mul(output.position, camera.view_matrix);
	output.position = mul(output.position, camera.projection_matrix);
	output.uv = input.uv;

	return output;
}