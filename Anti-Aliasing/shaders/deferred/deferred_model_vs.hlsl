cbuffer CameraBuffer : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
	float2 near_plane_vs_rectangle;
}

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
	float distance : TEXCOORD1;
};

VSOutput VS(VSInput input)
{
	VSOutput output;
	output.position = mul(float4(input.position, 1.0), view_matrix);
	output.distance = output.position.z;
	output.position = mul(output.position, projection_matrix);
	output.normal = mul(input.normal, (float3x3)view_matrix);
	output.uv = input.uv;
	return output;
}