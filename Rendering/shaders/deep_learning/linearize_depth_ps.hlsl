Texture2D<float> depth_buffer	: register(t0);

struct PSInput
{
	float4 position : SV_POSITION;
};

float linear_depth(float depth)
{
	float far = 100.0;
	float near = 0.1;
	depth = near * far / (far - depth * (far - near));
	return (depth - near) / (far - near);
}

float PS(PSInput input) : SV_TARGET
{
	uint2 pixel_pos = (uint2)input.position.xy;

	return linear_depth(depth_buffer[pixel_pos]);
}