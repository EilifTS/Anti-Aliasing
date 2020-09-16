#include "deferred_renderer.h"
#include "graphics/cpu_buffer.h"

namespace
{
	static const ema::vec3 world_light_dir = ema::vec3(1.0f, -1.0f, 1.0f).GetNormalized();
	struct lightBufferType
	{
		ema::vec4 light_dir;
	};
}

DeferrdRenderer::DeferrdRenderer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& size)
	: g_buffer(dev, size),
	light_buffer(dev, (int)sizeof(lightBufferType))
{
	context.SetTransitionBuffer(g_buffer.DepthBuffer(), egx::GPUBufferState::DepthWrite);

	initializeModelRenderer(dev, size);
	initializeModelNMRenderer(dev, size);
	initializeLightRenderer(dev, size);

}

void DeferrdRenderer::UpdateLight(egx::Camera& camera)
{
	light_dir = ema::vec4(world_light_dir, 0.0f) * camera.ViewMatrix();
}

void DeferrdRenderer::RenderModels(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::ModelList& models)
{
	context.SetTransitionBuffer(g_buffer.DiffuseBuffer(), egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(g_buffer.NormalBuffer(), egx::GPUBufferState::RenderTarget);

	context.ClearRenderTarget(g_buffer.DiffuseBuffer(), { 0.117f, 0.565f, 1.0f, 1.0f });
	context.ClearRenderTarget(g_buffer.NormalBuffer(), { 0.0f, 0.0f, 0.0f, 1.0f });
	context.ClearDepth(g_buffer.DepthBuffer(), 1.0f);
	context.SetRenderTargets(g_buffer.DiffuseBuffer(), g_buffer.NormalBuffer(), g_buffer.DepthBuffer());

	// Set root signature and pipeline state
	context.SetRootSignature(model_rs);
	context.SetPipelineState(model_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());

	// Set scissor and viewport
	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleList);

	for (auto pmesh : models)
	{
		if (pmesh->GetIndexBuffer().GetElementCount() > 0)
		{
			// Set vertex buffer
			context.SetVertexBuffer(pmesh->GetVertexBuffer());
			context.SetIndexBuffer(pmesh->GetIndexBuffer());

			// Set material
			const auto& material = pmesh->GetMaterial();
			context.SetRootConstantBuffer(1, material.GetBuffer());
			context.SetDescriptorHeap(*dev.buffer_heap);
			if (material.UseDiffuseTexture())
				context.SetRootDescriptorTable(2, material.GetDiffuseTexture());

			// Draw
			context.DrawIndexed(pmesh->GetIndexBuffer().GetElementCount());
		}
	}

}

void DeferrdRenderer::RenderModelsNM(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::ModelList& models)
{
	// Set root signature and pipeline state
	context.SetRootSignature(model_nm_rs);
	context.SetPipelineState(model_nm_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());

	// Set scissor and viewport
	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleList);

	for (auto pmesh : models)
	{
		if (pmesh->GetIndexBuffer().GetElementCount() > 0)
		{
			// Set vertex buffer
			context.SetVertexBuffer(pmesh->GetVertexBuffer());
			context.SetIndexBuffer(pmesh->GetIndexBuffer());

			// Set material
			const auto& material = pmesh->GetMaterial();
			context.SetRootConstantBuffer(1, material.GetBuffer());
			context.SetDescriptorHeap(*dev.buffer_heap);
			if (material.UseDiffuseTexture())
				context.SetRootDescriptorTable(2, material.GetDiffuseTexture());
			context.SetRootDescriptorTable(3, material.GetNormalMap());


			// Draw
			context.DrawIndexed(pmesh->GetIndexBuffer().GetElementCount());
		}
	}

}

void DeferrdRenderer::RenderLight(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::RenderTarget& target)
{
	// Update light buffer
	egx::CPUBuffer cpu_buffer(&light_dir, (int)sizeof(light_dir));
	context.SetTransitionBuffer(light_buffer, egx::GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, light_buffer);
	context.SetTransitionBuffer(light_buffer, egx::GPUBufferState::ConstantBuffer);

	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(g_buffer.DiffuseBuffer(), egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(g_buffer.NormalBuffer(), egx::GPUBufferState::PixelResource);

	context.ClearRenderTarget(target, { 0.117f, 0.565f, 1.0f, 1.0f });
	context.SetRenderTarget(target);

	// Set root signature and pipeline state
	context.SetRootSignature(light_rs);
	context.SetPipelineState(light_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootConstantBuffer(1, light_buffer);
	context.SetDescriptorHeap(*dev.buffer_heap);
	context.SetRootDescriptorTable(2, g_buffer.DiffuseBuffer());
	context.SetRootDescriptorTable(3, g_buffer.NormalBuffer());

	// Set scissor and viewport
	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	// Draw
	context.Draw(4);
}

void DeferrdRenderer::initializeModelRenderer(egx::Device& dev, const ema::point2D& size)
{
	// Create root signature
	model_rs.InitConstantBuffer(0);
	model_rs.InitConstantBuffer(1);
	model_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	model_rs.AddSampler(egx::Sampler::LinearWrap(), 0);
	model_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/deferred_model_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/deferred_model_ps.hlsl");

	// Get input layout
	auto input_layout = egx::MeshVertex::GetInputLayout();

	// Create PSO
	model_ps.SetRootSignature(model_rs);
	model_ps.SetInputLayout(input_layout);
	model_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	model_ps.SetVertexShader(VS);
	model_ps.SetPixelShader(PS);
	model_ps.SetDepthStencilFormat(g_buffer.DepthBuffer().Format());
	model_ps.SetRenderTargetFormats(g_buffer.DiffuseBuffer().Format(), g_buffer.NormalBuffer().Format());

	model_ps.SetBlendState(egx::BlendState::NoBlend());
	model_ps.SetRasterState(egx::RasterState::Default());
	model_ps.SetDepthStencilState(egx::DepthStencilState::DepthOn());
	model_ps.Finalize(dev);
}

void DeferrdRenderer::initializeModelNMRenderer(egx::Device& dev, const ema::point2D& size)
{
	// Create root signature
	model_nm_rs.InitConstantBuffer(0);
	model_nm_rs.InitConstantBuffer(1);
	model_nm_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	model_nm_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel);
	model_nm_rs.AddSampler(egx::Sampler::LinearWrap(), 0);
	model_nm_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/deferred_model_nm_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/deferred_model_nm_ps.hlsl");

	// Get input layout
	auto input_layout = egx::NormalMappedVertex::GetInputLayout();

	// Create PSO
	model_nm_ps.SetRootSignature(model_nm_rs);
	model_nm_ps.SetInputLayout(input_layout);
	model_nm_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	model_nm_ps.SetVertexShader(VS);
	model_nm_ps.SetPixelShader(PS);
	model_nm_ps.SetDepthStencilFormat(g_buffer.DepthBuffer().Format());
	model_nm_ps.SetRenderTargetFormats(g_buffer.DiffuseBuffer().Format(), g_buffer.NormalBuffer().Format());

	model_nm_ps.SetBlendState(egx::BlendState::NoBlend());
	model_nm_ps.SetRasterState(egx::RasterState::Default());
	model_nm_ps.SetDepthStencilState(egx::DepthStencilState::DepthOn());
	model_nm_ps.Finalize(dev);
}

void DeferrdRenderer::initializeLightRenderer(egx::Device& dev, const ema::point2D& size)
{
	// Create root signature
	light_rs.InitConstantBuffer(0);
	light_rs.InitConstantBuffer(1);
	light_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	light_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel);
	light_rs.AddSampler(egx::Sampler::PointClamp(), 0);
	light_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/deferred_light_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/deferred_light_ps.hlsl");

	// Create PSO
	light_ps.SetRootSignature(light_rs);
	//light_ps.SetInputLayout(NULL);
	light_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	light_ps.SetVertexShader(VS);
	light_ps.SetPixelShader(PS);
	//light_ps.SetDepthStencilFormat(NULL);
	light_ps.SetRenderTargetFormats(g_buffer.DiffuseBuffer().Format(), g_buffer.NormalBuffer().Format());

	light_ps.SetBlendState(egx::BlendState::NoBlend());
	light_ps.SetRasterState(egx::RasterState::Default());
	light_ps.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	light_ps.Finalize(dev);
}