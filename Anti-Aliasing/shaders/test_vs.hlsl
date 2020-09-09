cbuffer CameraBuffer : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
	float4 view_direction;
}

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float3 color : COLOR;
	float specular : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
	float3 normal : NORMAL;
	float3 world_pos : TEXCOORD0;
	float spec_exp : TEXCOORD1;
};

VSOutput VS( VSInput input )
{
	VSOutput output;
	output.world_pos = input.position;
	output.position = mul(float4(input.position, 1.0), view_matrix);
	output.position = mul(output.position, projection_matrix);
	output.normal = input.normal;
	output.color = input.color;
	output.spec_exp = input.specular;
	return output;
}