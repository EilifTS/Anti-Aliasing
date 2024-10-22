#include "../headers/camera.hlsli"
#include "../headers/model.hlsli"

cbuffer CameraBuffer : register(b0)
{
	CameraBufferType curr_cam;
}
cbuffer LastFrameCameraBuffer : register(b1)
{
	CameraBufferType last_cam;
}
cbuffer ModelBuffer : register(b2)
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
	float3 curr_pos: TEXCOORD0;
	float3 last_pos: TEXCOORD1;
};

VSOutput VS(VSInput input)
{
	VSOutput output;
	float4 curr_pos = mul(float4(input.position, 1.0), model.world_matrix);
	float4 last_pos = mul(float4(input.position, 1.0), model.last_world_matrix);
	curr_pos = mul(curr_pos, curr_cam.view_matrix);
	last_pos = mul(last_pos, last_cam.view_matrix);
	output.position = mul(curr_pos, curr_cam.projection_matrix);
	curr_pos = mul(curr_pos, curr_cam.projection_matrix_no_jitter);
	last_pos = mul(last_pos, last_cam.projection_matrix_no_jitter);
	
	// This part does not work when one of the vertices are behind the camera
	//curr_pos /= curr_pos.w;
	//last_pos /= last_pos.w;
	//output.mv = last_pos.xy - curr_pos.xy;
	//output.mv = output.mv * float2(0.5, -0.5);
	output.curr_pos = curr_pos.xyw;
	output.last_pos = last_pos.xyw;


	return output;
}