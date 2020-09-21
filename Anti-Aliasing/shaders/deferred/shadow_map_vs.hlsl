cbuffer CameraBuffer : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
	matrix inv_projection_matrix;
	matrix inv_projection_matrix_no_jitter;
}
cbuffer ModelBuffer : register(b1)
{
	matrix world_matrix;
	uint is_static;
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
	output.position = mul(float4(input.position, 1.0), world_matrix);
	output.position = mul(output.position, view_matrix);
	output.position = mul(output.position, projection_matrix);
	output.uv = input.uv;

	return output;
}