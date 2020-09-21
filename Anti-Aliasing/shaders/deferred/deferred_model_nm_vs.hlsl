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
	float distance : TEXCOORD1;
	float3x3 tbn : NORMAL;
};

VSOutput VS(VSInput input)
{
	VSOutput output;
	output.position = mul(float4(input.position, 1.0), world_matrix);
	output.position = mul(output.position, view_matrix);
	output.distance = output.position.z;
	output.position = mul(output.position, projection_matrix);

	float3 view_tangent		= mul(input.tangent.xyz, (float3x3)world_matrix);
	view_tangent			= normalize(mul(view_tangent, (float3x3)view_matrix));
	float3 view_normal		= mul(input.normal, (float3x3)world_matrix);
	view_normal				= normalize(mul(view_normal, (float3x3)view_matrix));
	float3 view_bitangent	= cross(view_tangent, view_normal) * input.tangent.w;
	output.tbn = float3x3(view_tangent, view_bitangent, view_normal);

	output.uv = input.uv;

	return output;
}