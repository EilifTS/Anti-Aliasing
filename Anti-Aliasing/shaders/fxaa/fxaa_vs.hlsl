
struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

VSOutput VS( uint id : SV_VertexID )
{
	VSOutput output;
	output.uv = float2((id << 1) & 2, id & 2);
	output.position = float4(output.uv * float2(2.0, -2.0) - float2(1.0, -1.0), 0.0, 1.0);

	return output;
}