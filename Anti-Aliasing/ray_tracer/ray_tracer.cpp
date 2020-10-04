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

	// Setup root signatures
	compute_rs.Finalize(dev);
	ray_gen_rs.InitUnorderedAccessTable(0, 1, egx::ShaderVisibility::All); // Output
	ray_gen_rs.InitDescriptorTable(0); // Acceleration structure
	ray_gen_rs.InitConstantBuffer(0); // Camera buffer
	ray_gen_rs.Finalize(dev, true);

	miss_rs.Finalize(dev, true);

	hit_rs.InitShaderResource(1); // Vertex Bufffer
	hit_rs.InitShaderResource(2); // Index Buffer
	hit_rs.Finalize(dev, true);

	egx::ShaderLibrary lib;
	lib.Compile("shaders/ray_tracing/shader.hlsl");

	pipeline_state.AddLibrary(lib, { L"RayGenerationShader", L"MissShader", L"ClosestHitShader" });
	pipeline_state.AddHitGroup(L"HitGroup1", L"ClosestHitShader");
	pipeline_state.AddRootSignatureAssociation(ray_gen_rs, { L"RayGenerationShader" });
	pipeline_state.AddRootSignatureAssociation(miss_rs, { L"MissShader" });
	pipeline_state.AddRootSignatureAssociation(hit_rs, { L"HitGroup1" });
	pipeline_state.AddGlobalRootSignature(compute_rs);
	pipeline_state.SetMaxPayloadSize(3 * sizeof(float));
	pipeline_state.SetMaxAttributeSize(2 * sizeof(float));
	pipeline_state.SetMaxRecursionDepth(1);
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

void RayTracer::UpdateShaderTable(egx::Device& dev, egx::ConstantBuffer& camera_buffer, std::vector<std::shared_ptr<egx::Mesh>>& meshes)
{
	auto& program1 = shader_table.AddRayGenerationProgram(L"RayGenerationShader");
	program1.AddUnorderedAccessTable(output_buffer);
	program1.AddAccelerationStructure(tlas);
	program1.AddConstantBuffer(camera_buffer);
	auto& program2 = shader_table.AddMissProgram(L"MissShader");

	int test_instance_id = 0;
	for (auto mesh : meshes)
	{
		assert(mesh->GetInstanceID() == test_instance_id++);
		auto& program3 = shader_table.AddHitProgram(L"HitGroup1");
		program3.AddVertexBuffer(mesh->GetVertexBuffer());
		program3.AddIndexBuffer(mesh->GetIndexBuffer());
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