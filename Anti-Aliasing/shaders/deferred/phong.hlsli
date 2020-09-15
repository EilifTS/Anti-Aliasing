float phong_diffuse_light(float3 light_dir, float3 normal)
{
	return saturate(-dot(light_dir, normal));
}

float3 phong_reflection_vector(float3 light_dir, float3 normal)
{
	return light_dir - 2 * dot(light_dir, normal) * normal;
}

float phong_specular_light(float3 reflection_vector, float3 view_dir, float specular_exp)
{
	float specular = 0.0f;
	if (specular_exp > 0.0)
		specular = pow(saturate(-dot(reflection_vector, view_dir)), specular_exp);;
	return specular;
}