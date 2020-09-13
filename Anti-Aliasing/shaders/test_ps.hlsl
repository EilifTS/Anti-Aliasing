cbuffer CameraBuffer : register(b0)
{
	matrix view_matrix;
	matrix projection_matrix;
	float4 camera_position;
}

cbuffer MaterialBuffer : register(b1)
{
	float4 diffuse_color;
	float specular_exponent;
}

struct PSInput
{
	float4 position : SV_POSITION;
	float3 world_pos : TEXCOORD0;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD1;
};

float4 PS(PSInput input) : SV_TARGET
{
	float3 color = diffuse_color.rgb;
	float3 normal = normalize(input.normal);
	float3 light_dir = normalize(float3(1.0, -1.0, 1.0));

	float diffuse = saturate(-dot(light_dir, normal));
	
	float3 reflection_vector = light_dir - 2 * dot(light_dir, normal) * normal;
	
	float3 view_dir = normalize(input.world_pos - camera_position.xyz);

	float specular =  pow(saturate(-dot(reflection_vector, view_dir)), specular_exponent);
	if (specular_exponent <= 0.0)
		specular = 0.0;

	color = color * (0.5 + 0.5 * diffuse) + 0.5 * specular.xxx;

	return float4(color, 1.0);
}