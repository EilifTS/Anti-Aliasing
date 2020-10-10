#pragma once
#include "graphics/unordered_access_buffer.h"
#include "graphics/ray_tracing/tlas.h"
#include "graphics/ray_tracing/rt_pipeline_state.h"
#include "graphics/ray_tracing/shader_table.h"
#include "graphics/root_signature.h"
#include "graphics/device.h"
#include "graphics/command_context.h"

class RayTracer
{
public:
	RayTracer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& window_size);

	void BuildTLAS(egx::Device& dev, egx::CommandContext& context, std::vector<std::shared_ptr<egx::Model>>& models);
	void ReBuildTLAS(egx::CommandContext& context, std::vector<std::shared_ptr<egx::Model>>& models);
	void UpdateShaderTable(egx::Device& dev, egx::ConstantBuffer& camera_buffer, egx::ConstantBuffer& jitter_buffer, std::vector<std::shared_ptr<egx::Mesh>>& meshes);

	egx::UnorderedAccessBuffer& Trace(egx::Device& dev, egx::CommandContext& context);

private:
	ema::point2D window_size;
	egx::UnorderedAccessBuffer output_buffer;
	egx::TLAS tlas;
	egx::RootSignature compute_rs;
	egx::RootSignature ray_gen_rs;
	egx::RootSignature miss_rs;
	egx::RootSignature hit_rs;
	egx::RootSignature shadowhit_rs;
	egx::RTPipelineState pipeline_state;
	egx::ShaderTable shader_table;
};