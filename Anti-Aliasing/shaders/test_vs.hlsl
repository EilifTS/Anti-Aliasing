struct VSInput
{
	float3 position : POSITION;
	float3 normal : POSITION;
	float3 color : COLOR;
	float specular : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VSOutput VS( VSInput input )
{
	VSOutput output;
	output.position = float4(input.position*0.03 + float3(0.0, 0.0, 0.5), 1.0);
	output.color = float4(input.color, 1.0);
	return output;
}