float phong_diffuse_light(float3 light_dir, float3 normal)
{
	return saturate(dot(light_dir, normal));
}

float3 phong_reflection_vector(float3 light_dir, float3 normal)
{
	return light_dir - 2 * dot(light_dir, normal) * normal;
}

float phong_specular_light(float3 reflection_vector, float3 view_dir, float specular_exp)
{
	float specular = 0.0;
	if (specular_exp > 0.0)
		specular = pow(saturate(dot(reflection_vector, view_dir)), specular_exp);;
	return specular;
}

float specular(float3 n, float3 h, float r)
{
	float alpha = r * r;
	float alpha2 = alpha * alpha;

	float nh = dot(n, h);

	const float pi = 3.14159265f;

	// D_gxx
	float t = nh * nh * (alpha2 - 1.0) + 1.0;
	float D = alpha2 / (pi * t * t);
	return D;
}

float specular_spec(float3 n, float3 l, float3 v, float3 h, float r)
{
	float alpha = r * r;
	float alpha2 = alpha * alpha;

	const float pi = 3.14159265f;
	float k = (r+1.0)*(r+1.0) * 0.125;
	float nh = dot(n, h);
	float nv = dot(n, v);
	float nl = dot(n, l);
	float vh = dot(v, h);


	// D_gxx
	float t = nh * nh * (alpha2 - 1.0) + 1.0;
	float D = alpha2 / (pi * t * t);
	return D;

	// G_schlick
	float G_v = 1.0 / (nv * (1.0 - k) + k);
	float G_l = 1.0 / (nl * (1.0 - k) + k);
	//float G_ct = min(1.0, min(2.0 * nh * nv / vh, 2.0 * nh * nl / vh));
	//float G = G_ct / (nv * nl);
	float G = G_v * G_l;

	// Fresnel
	float F_0 = 0.04;
	float power = (-5.55473 * vh - 6.98316) * vh;
	float F = F_0 + ldexp(1.0 - F_0, power);

	return D * G * F * 0.25;
}