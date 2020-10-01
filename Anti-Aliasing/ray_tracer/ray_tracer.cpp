#include "ray_tracer.h"


RayTracer::RayTracer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& window_size)
	: window_size(window_size),
	output_buffer(dev, egx::TextureFormat::UNORM8x4, window_size),
	tlas(),
	compute_rs(), ray_gen_rs(), miss_rs(), hit_rs(),
	pipeline_state(), shader_table()
{
	output_buffer.CreateUnorderedAccessView(dev);

	// Add scene and output buffer
	compute_rs.Finalize(dev);
	ray_gen_rs.InitUnorderedAccessTable(0, 1, egx::ShaderVisibility::All);
	//ray_gen_rs.InitDescriptorTable(0);
	//ray_gen_rs.InitShaderResource(0);
	ray_gen_rs.Finalize(dev, true); // Set local
	miss_rs.Finalize(dev, true);
	hit_rs.Finalize(dev, true);

	egx::ShaderLibrary lib;
	lib.Compile("shaders/ray_tracing/shader.hlsl");

	pipeline_state.AddLibrary(lib, { L"RayGenerationShader", L"MissShader", L"ClosestHitShader" });
	pipeline_state.AddHitGroup(L"HitGroup1", L"ClosestHitShader");
	pipeline_state.AddRootSignatureAssociation(ray_gen_rs, { L"RayGenerationShader" });
	pipeline_state.AddRootSignatureAssociation(miss_rs, { L"MissShader", L"HitGroup1" });
	//pipeline_state.AddRootSignatureAssociation(hit_rs, { L"HitGroup1" });
	pipeline_state.AddGlobalRootSignature(compute_rs);
	pipeline_state.SetMaxPayloadSize(3 * sizeof(float));
	pipeline_state.SetMaxAttributeSize(2 * sizeof(float));
	pipeline_state.SetMaxRecursionDepth(1);
	pipeline_state.Finalize(dev);

	
}

void RayTracer::UpdateTLAS(egx::Device& dev, egx::CommandContext& context, std::vector<std::shared_ptr<egx::Model>>& models)
{
	tlas.Build(dev, context, models);
}

void RayTracer::UpdateShaderTable(egx::Device& dev)
{
	auto& program1 = shader_table.AddRayGenerationProgram(L"RayGenerationShader");
	program1.AddUnorderedAccessTable(output_buffer);
	//program1.AddAccelerationStructure(tlas);
	auto& program2 = shader_table.AddMissProgram(L"MissShader");
	auto& program3 = shader_table.AddHitProgram(L"HitGroup1");
	shader_table.Finalize(dev, pipeline_state);
}


egx::UnorderedAccessBuffer& RayTracer::Trace(egx::Device& dev, egx::CommandContext& context)
{
	context.SetTransitionBuffer(output_buffer, egx::GPUBufferState::UnorderedAccess);
	context.SetComputeRootSignature(compute_rs);
	context.SetRTPipelineState(pipeline_state);
	context.DispatchRays(window_size, shader_table);
	return output_buffer;
}