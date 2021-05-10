Texture2D input_texture	: register(t0);

struct PSInput
{
	float4 position : SV_POSITION;
};



float4 PS(PSInput input) : SV_TARGET
{
	uint2 pixel_pos = (uint2)input.position.xy;

	return pow(input_texture[pixel_pos], 1.0 / 2.2);
}