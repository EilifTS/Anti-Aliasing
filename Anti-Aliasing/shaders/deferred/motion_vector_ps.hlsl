// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float2 mv : TEXCOORD;
};

float2 PS(PSInput input) : SV_TARGET
{
	return input.mv;
}