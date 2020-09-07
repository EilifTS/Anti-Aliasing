struct VSInput
{
	float3 position : POSITION;
	float4 color : COLOR;
};

struct VSOutput
{
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

VSOutput VS( VSInput input )
{
	VSOutput output;
	output.position = float4(input.position, 1.0);
	output.color = input.color;
	return output;
}