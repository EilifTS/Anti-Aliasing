#include "ray_tracer.h"
#include "graphics/mesh.h"

RayTracer::RayTracer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& window_size)
	: window_size(window_size),
	output_buffer(dev, egx::TextureFormat::UNORM8x4, window_size),
	tlas(),
	compute_rs(), ray_gen_rs(), miss_rs(), hit_rs(),
	pipeline_state(), shader_table()
{
	output_buffer.CreateUnorderedAccessView(dev);

	// Create samplers
	auto sampler = egx::Sampler::LinearWrap();
	sampler.SetVisibility(egx::ShaderVisibility::All);

	// Setup root signatures
	compute_rs.Finalize(dev);
	ray_gen_rs.InitUnorderedAccessTable(0, 1, egx::ShaderVisibility::All); // Output
	ray_gen_rs.InitDescriptorTable(0); // Acceleration structure
	ray_gen_rs.InitConstantBuffer(0); // Camera buffer
	ray_gen_rs.InitConstantBuffer(1); // Jitter buffer
	ray_gen_rs.Finalize(dev, true);

	miss_rs.Finalize(dev, true);

	hit_rs.InitShaderResource(0); // Vertex Bufffer
	hit_rs.InitShaderResource(1); // Index Buffer
	hit_rs.InitConstantBuffer(0); // Material buffer
	hit_rs.InitDescriptorTable(2); // Diffuse color texture
	hit_rs.InitDescriptorTable(3); // Normal map
	hit_rs.InitDescriptorTable(4); // Specular map
	hit_rs.InitDescriptorTable(5); // Mask texture
	hit_rs.InitDescriptorTable(6); // Acceleration structure
	hit_rs.AddSampler(sampler, 0);
	hit_rs.Finalize(dev, true);

	shadowhit_rs.InitShaderResource(0); // Vertex Bufffer
	shadowhit_rs.InitShaderResource(1); // Index Buffer
	shadowhit_rs.InitConstantBuffer(0); // Material buffer
	shadowhit_rs.InitDescriptorTable(2); // Mask texture
	shadowhit_rs.AddSampler(sampler, 0);
	shadowhit_rs.Finalize(dev, true);

	egx::ShaderLibrary lib1;
	egx::ShaderLibrary lib2;
	egx::ShaderLibrary lib3;
	egx::ShaderLibrary lib4;
	lib1.Compile("../Rendering/shaders/ray_tracing/ray_gen.hlsl");
	lib2.Compile("../Rendering/shaders/ray_tracing/miss.hlsl");
	lib3.Compile("../Rendering/shaders/ray_tracing/closest_hit.hlsl");
	lib4.Compile("../Rendering/shaders/ray_tracing/shadow_ray.hlsl");

	pipeline_state.AddLibrary(lib1, { L"RayGenerationShader" });
	pipeline_state.AddLibrary(lib2, { L"MissShader" });
	pipeline_state.AddLibrary(lib3, { L"AnyHitShader", L"ClosestHitShader" });
	pipeline_state.AddLibrary(lib4, { L"ShadowMiss", L"ShadowAnyHit", L"ShadowCHS" });
	pipeline_state.AddHitGroup(L"HitGroup1", L"ClosestHitShader", L"AnyHitShader");
	pipeline_state.AddHitGroup(L"ShadowHitGroup", L"ShadowCHS", L"ShadowAnyHit");
	pipeline_state.AddRootSignatureAssociation(ray_gen_rs, { L"RayGenerationShader" });
	pipeline_state.AddRootSignatureAssociation(miss_rs, { L"MissShader", L"ShadowMiss" });
	pipeline_state.AddRootSignatureAssociation(hit_rs, { L"HitGroup1" });
	pipeline_state.AddRootSignatureAssociation(shadowhit_rs, { L"ShadowHitGroup" });
	pipeline_state.AddGlobalRootSignature(compute_rs);
	pipeline_state.SetMaxPayloadSize(3 * sizeof(float) + sizeof(int));
	pipeline_state.SetMaxAttributeSize(2 * sizeof(float));
	pipeline_state.SetMaxRecursionDepth(3);
	pipeline_state.Finalize(dev);

	
}

void RayTracer::BuildTLAS(egx::Device& dev, egx::CommandContext& context, std::vector<std::shared_ptr<egx::Model>>& models)
{
	tlas.Build(dev, context, models);
}
void RayTracer::ReBuildTLAS(egx::CommandContext& context, std::vector<std::shared_ptr<egx::Model>>& models)
{
	tlas.ReBuild(context, models);
}

void RayTracer::UpdateShaderTable(egx::Device& dev, egx::ConstantBuffer& camera_buffer, egx::ConstantBuffer& jitter_buffer, std::vector<std::shared_ptr<egx::Mesh>>& meshes)
{
	auto& program1 = shader_table.AddRayGenerationProgram(L"RayGenerationShader");
	program1.AddUnorderedAccessTable(output_buffer);
	program1.AddAccelerationStructure(tlas);
	program1.AddConstantBuffer(camera_buffer);
	program1.AddConstantBuffer(jitter_buffer);
	auto& program2 = shader_table.AddMissProgram(L"MissShader");
	auto& program3 = shader_table.AddMissProgram(L"ShadowMiss");

	int test_instance_id = 0;
	for (auto mesh : meshes)
	{
		assert(mesh->GetInstanceID() == test_instance_id++);
		const auto& material = mesh->GetMaterial();

		auto& program4 = shader_table.AddHitProgram(L"HitGroup1");
		program4.AddVertexBuffer(mesh->GetVertexBuffer());
		program4.AddIndexBuffer(mesh->GetIndexBuffer());
		program4.AddConstantBuffer(material.GetBuffer());

		if (material.HasDiffuseTexture())
			program4.AddDescriptorTable(material.GetDiffuseTexture());
		else
			program4.AddNullDescriptor();
		if(material.HasNormalMap())
			program4.AddDescriptorTable(material.GetNormalMap());
		else
			program4.AddNullDescriptor();
		if(material.HasSpecularMap())
			program4.AddDescriptorTable(material.GetSpecularMap());
		else
			program4.AddNullDescriptor();
		if(material.HasMaskTexture())
			program4.AddDescriptorTable(material.GetMaskTexture());
		else
			program4.AddNullDescriptor();
		program4.AddAccelerationStructure(tlas);

		// Shadow hit group
		auto& program5 = shader_table.AddHitProgram(L"ShadowHitGroup");
		program5.AddVertexBuffer(mesh->GetVertexBuffer());
		program5.AddIndexBuffer(mesh->GetIndexBuffer());
		program5.AddConstantBuffer(material.GetBuffer());
		if (material.HasMaskTexture())
			program5.AddDescriptorTable(material.GetMaskTexture());
		else
			program5.AddNullDescriptor();
	}
	shader_table.Finalize(dev, pipeline_state);
}


egx::UnorderedAccessBuffer& RayTracer::Trace(egx::Device& dev, egx::CommandContext& context)
{
	context.SetDescriptorHeap(*(dev.buffer_heap));
	context.SetTransitionBuffer(output_buffer, egx::GPUBufferState::UnorderedAccess);
	context.SetComputeRootSignature(compute_rs);
	context.SetRTPipelineState(pipeline_state);
	context.DispatchRays(window_size, shader_table);
	return output_buffer;
}