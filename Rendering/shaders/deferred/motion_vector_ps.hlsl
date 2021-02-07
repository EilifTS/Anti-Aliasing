// Pixel shader input
struct PSInput
{
	float4 position : SV_POSITION;
	float3 curr_pos : TEXCOORD0;
	float3 last_pos : TEXCOORD1;
};

float2 PS(PSInput input) : SV_TARGET
{
	input.curr_pos /= input.curr_pos.z;
	input.last_pos /= input.last_pos.z;
	float2 mv = input.last_pos.xy - input.curr_pos.xy;
	mv = mv * float2(0.5, -0.5);
	return mv;
}