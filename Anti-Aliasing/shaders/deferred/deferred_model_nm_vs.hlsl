cbuffer CameraBuffer : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
	float2 near_plane_vs_rectangle;
}

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL0;
	float3 tangent : NORMAL1;
	float3 bitangent : NORMAL2;
	float2 uv : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
	float distance : TEXCOORD1;
	float3 normal : NORMAL0;
	float3x3 tbn : NORMAL1;
};

VSOutput VS(VSInput input)
{
	VSOutput output;
	output.position = mul(float4(input.position, 1.0), view_matrix);
	output.distance = output.position.z;
	output.position = mul(output.position, projection_matrix);

	float3 view_tangent		= normalize(mul(input.tangent, (float3x3)view_matrix));
	float3 view_normal		= normalize(mul(input.normal, (float3x3)view_matrix));
	float3 view_bitangent	= normalize(mul(input.bitangent, (float3x3)view_matrix));
	//float3 view_bitangent	= -cross(view_tangent, view_normal);
	output.tbn = float3x3(view_tangent, view_bitangent, view_normal);
	output.normal = view_normal;

	output.uv = input.uv;

	return output;
}