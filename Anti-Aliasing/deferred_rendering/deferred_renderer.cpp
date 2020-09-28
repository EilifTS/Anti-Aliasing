#include "deferred_renderer.h"
#include "graphics/cpu_buffer.h"

DeferrdRenderer::DeferrdRenderer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& size, float far_plane)
	: g_buffer(dev, size, far_plane),
	light_manager(dev, context),
	tone_mapper(dev),
	motion_vectors(dev, egx::TextureFormat::FLOAT16x2, size)
{
	context.SetTransitionBuffer(g_buffer.DepthBuffer(), egx::GPUBufferState::DepthWrite);

	motion_vectors.CreateShaderResourceView(dev);
	motion_vectors.CreateRenderTargetView(dev);

	initializeModelRenderer(dev);
	initializeLightRenderer(dev);
	initializeMotionVectorRenderer(dev);

}

void DeferrdRenderer::UpdateLight(egx::Camera& camera)
{
	light_manager.Update(camera);
}


void DeferrdRenderer::PrepareFrame(egx::Device& dev, egx::CommandContext& context)
{
	if (recompile_shaders) recompileShaders(dev);

	light_manager.PrepareFrame(dev, context);

	context.SetTransitionBuffer(g_buffer.DiffuseBuffer(), egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(g_buffer.NormalBuffer(), egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(g_buffer.DepthBuffer(), egx::GPUBufferState::DepthWrite);
	context.SetTransitionBuffer(motion_vectors, egx::GPUBufferState::RenderTarget);

	context.ClearRenderTarget(g_buffer.DiffuseBuffer());
	context.ClearRenderTarget(g_buffer.NormalBuffer());
	context.ClearRenderTarget(motion_vectors);
	context.ClearDepthStencil(g_buffer.DepthBuffer());
}

void DeferrdRenderer::RenderModel(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model)
{
	light_manager.RenderToShadowMap(dev, context, model);

	context.SetRenderTargets(g_buffer.DiffuseBuffer(), g_buffer.NormalBuffer(), g_buffer.DepthBuffer());

	// Set root signature and pipeline state
	context.SetRootSignature(model_rs);
	context.SetPipelineState(model_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootConstantBuffer(1, model.GetModelBuffer());

	// Set scissor and viewport
	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleList);

	for (auto pmesh : model.GetMeshes())
	{
		if (pmesh->GetIndexBuffer().GetElementCount() > 0)
		{
			// Set vertex buffer
			context.SetVertexBuffer(pmesh->GetVertexBuffer());
			context.SetIndexBuffer(pmesh->GetIndexBuffer());

			// Set material
			const auto& material = pmesh->GetMaterial();
			context.SetRootConstantBuffer(2, material.GetBuffer());
			context.SetDescriptorHeap(*dev.buffer_heap);

			if (material.HasDiffuseTexture())
				context.SetRootDescriptorTable(3, material.GetDiffuseTexture());
			if (material.HasNormalMap())
				context.SetRootDescriptorTable(4, material.GetNormalMap());
			if (material.HasSpecularMap())
				context.SetRootDescriptorTable(5, material.GetSpecularMap());
			if (material.HasMaskTexture())
				context.SetRootDescriptorTable(6, material.GetMaskTexture());

			// Draw
			context.DrawIndexed(pmesh->GetIndexBuffer().GetElementCount());
		}
	}

}

void DeferrdRenderer::RenderLight(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::RenderTarget& target)
{
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(g_buffer.DiffuseBuffer(), egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(g_buffer.NormalBuffer(), egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(light_manager.GetShadowMap(), egx::GPUBufferState::PixelResource);

	context.ClearRenderTarget(target);
	context.SetRenderTarget(target);

	// Set root signature and pipeline state
	context.SetRootSignature(light_rs);
	context.SetPipelineState(light_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootConstantBuffer(1, light_manager.GetLightBuffer());
	context.SetDescriptorHeap(*dev.buffer_heap);
	context.SetRootDescriptorTable(2, g_buffer.DiffuseBuffer());
	context.SetRootDescriptorTable(3, g_buffer.NormalBuffer());
	context.SetRootDescriptorTable(4, light_manager.GetShadowMap());

	// Set scissor and viewport
	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	// Draw
	context.Draw(4);
}

void DeferrdRenderer::RenderMotionVectors(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model)
{
	context.SetRenderTarget(motion_vectors, g_buffer.DepthBuffer());

	// Set root signature and pipeline state
	context.SetRootSignature(motion_vector_rs);
	context.SetPipelineState(motion_vector_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootConstantBuffer(1, camera.GetLastBuffer());
	context.SetRootConstantBuffer(2, model.GetModelBuffer());

	// Set scissor and viewport
	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleList);
	context.SetStencilRefrenceValue(1);

	for (auto pmesh : model.GetMeshes())
	{
		if (pmesh->GetIndexBuffer().GetElementCount() > 0)
		{
			// Set vertex buffer
			context.SetVertexBuffer(pmesh->GetVertexBuffer());
			context.SetIndexBuffer(pmesh->GetIndexBuffer());

			// Draw
			context.DrawIndexed(pmesh->GetIndexBuffer().GetElementCount());
		}
	}
}

void DeferrdRenderer::SetSampler(bool biased)
{
	if (biased)
	{
		macro_list.SetMacro("SAMPLER", "linear_wrap_biased");
	}
	else
	{
		macro_list.SetMacro("SAMPLER", "linear_wrap");
	}
	recompile_shaders = true;
}

void DeferrdRenderer::initializeModelRenderer(egx::Device& dev)
{
	// Create root signature
	auto biased_sampler = egx::Sampler::LinearWrap();
	biased_sampler.SetMipMapBias(-1.0f);

	model_rs.InitConstantBuffer(0); // Camera buffer
	model_rs.InitConstantBuffer(1); // Model buffer
	model_rs.InitConstantBuffer(2); // Material buffer
	model_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // Diffuse
	model_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // Normals
	model_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // Specular
	model_rs.InitDescriptorTable(3, egx::ShaderVisibility::Pixel); // Mask
	model_rs.AddSampler(egx::Sampler::LinearWrap(), 0);
	model_rs.AddSampler(biased_sampler, 1);
	model_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/deferred_model_nm_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/deferred_model_nm_ps.hlsl");

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

void DeferrdRenderer::initializeLightRenderer(egx::Device& dev)
{
	// Create root signature
	light_rs.InitConstantBuffer(0);
	light_rs.InitConstantBuffer(1);
	light_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // Diffuse texture
	light_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // Normal texture
	light_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // Shadow map
	light_rs.AddSampler(egx::Sampler::PointClamp(), 0);
	light_rs.AddSampler(egx::Sampler::ShadowSampler(), 1);
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

void DeferrdRenderer::initializeMotionVectorRenderer(egx::Device& dev)
{
	// Create root signature
	motion_vector_rs.InitConstantBuffer(0); // Camera buffer
	motion_vector_rs.InitConstantBuffer(1); // Last Camera buffer
	motion_vector_rs.InitConstantBuffer(2); // Model buffer
	motion_vector_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	motion_vector_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/motion_vector_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/motion_vector_ps.hlsl");

	// Input layout
	egx::InputLayout layout = egx::MeshVertex::GetInputLayout();

	// Create PSO
	motion_vector_ps.SetRootSignature(motion_vector_rs);
	motion_vector_ps.SetInputLayout(layout);
	motion_vector_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	motion_vector_ps.SetVertexShader(VS);
	motion_vector_ps.SetPixelShader(PS);
	motion_vector_ps.SetDepthStencilFormat(g_buffer.DepthBuffer().Format());
	motion_vector_ps.SetRenderTargetFormat(motion_vectors.Format());

	motion_vector_ps.SetBlendState(egx::BlendState::NoBlend());
	motion_vector_ps.SetRasterState(egx::RasterState::Default());
	motion_vector_ps.SetDepthStencilState(egx::DepthStencilState::MotionVectorWriteStencil());
	motion_vector_ps.Finalize(dev);
}

void DeferrdRenderer::recompileShaders(egx::Device& dev)
{
	dev.WaitForGPU();
	auto input_layout = egx::MeshVertex::GetInputLayout();
	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/deferred_model_nm_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/deferred_model_nm_ps.hlsl", macro_list);
	model_ps.SetInputLayout(input_layout);
	model_ps.SetVertexShader(VS);
	model_ps.SetPixelShader(PS);
	model_ps.Finalize(dev);
	recompile_shaders = false;
}