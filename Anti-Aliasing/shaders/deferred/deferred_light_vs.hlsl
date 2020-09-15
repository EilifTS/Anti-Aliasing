cbuffer CameraBuffer : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
	float2 near_plane_vs_rectangle;
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
	output.view_position = float3(output.position.xy * near_plane_vs_rectangle, 1.0);

	return output;
}