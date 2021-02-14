#include "deferred_renderer.h"
#include "graphics/cpu_buffer.h"

DeferredRenderer::DeferredRenderer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& size, float far_plane, float mipmap_bias)
	: g_buffer(dev, size, far_plane),
	size(size),
	light_manager(dev, context),
	tone_mapper(dev),
	motion_vectors(dev, egx::TextureFormat::FLOAT16x2, size)
{
	context.SetTransitionBuffer(g_buffer.DepthBuffer(), egx::GPUBufferState::DepthWrite);

	motion_vectors.CreateShaderResourceView(dev);
	motion_vectors.CreateRenderTargetView(dev);

	initializeDepthOnlyRenderer(dev);
	initializeModelRenderer(dev, mipmap_bias);
	initializeLightRenderer(dev);
	initializeMotionVectorRenderer(dev);
}

void DeferredRenderer::UpdateLight(egx::Camera& camera)
{
	light_manager.Update(camera);
}


void DeferredRenderer::PrepareFrame(egx::Device& dev, egx::CommandContext& context)
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

void DeferredRenderer::RenderDepthOnly(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model)
{
	context.SetDepthStencilBuffer(g_buffer.DepthBuffer());

	// Set root signature and pipeline state
	context.SetRootSignature(depth_only_rs);
	context.SetPipelineState(depth_only_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootConstantBuffer(1, model.GetModelBuffer());

	// Set scissor and viewport
	context.SetViewport(size);
	context.SetScissor(size);
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

			if (material.HasMaskTexture())
				context.SetRootDescriptorTable(3, material.GetMaskTexture());

			// Draw
			context.DrawIndexed(pmesh->GetIndexBuffer().GetElementCount());
		}
	}
}

void DeferredRenderer::RenderModel(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model)
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
	context.SetViewport(size);
	context.SetScissor(size);
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

void DeferredRenderer::RenderLight(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::RenderTarget& target)
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
	context.SetRootDescriptorTable(2, g_buffer.DiffuseBuffer());
	context.SetRootDescriptorTable(3, g_buffer.NormalBuffer());
	context.SetRootDescriptorTable(4, light_manager.GetShadowMap());

	// Set scissor and viewport
	context.SetViewport(size);
	context.SetScissor(size);
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	// Draw
	context.Draw(4);
}

void DeferredRenderer::RenderMotionVectors(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model)
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
	context.SetViewport(size);
	context.SetScissor(size);
	context.SetPrimitiveTopology(egx::Topology::TriangleList);

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

void DeferredRenderer::SetSampler(TextureSampler sampler)
{
	switch (sampler)
	{
	case DeferredRenderer::TextureSampler::SSAABias:
		macro_list.SetMacro("SAMPLER", "linear_wrap_ssaa_bias");
		break;
	case DeferredRenderer::TextureSampler::TAABias:
		macro_list.SetMacro("SAMPLER", "linear_wrap_taa_bias");
		break;
	case DeferredRenderer::TextureSampler::NoBias:
		macro_list.SetMacro("SAMPLER", "linear_wrap_no_bias");
		break;
	default:
		break;
	}
	recompile_shaders = true;
}

void DeferredRenderer::initializeDepthOnlyRenderer(egx::Device& dev)
{
	// Create root signature
	depth_only_rs.InitConstantBuffer(0); // Camera
	depth_only_rs.InitConstantBuffer(1); // Model
	depth_only_rs.InitConstantBuffer(2); // Material
	depth_only_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // Material mask texture
	depth_only_rs.AddSampler(egx::Sampler::LinearWrap(), 0);
	depth_only_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/deferred/depth_only_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/deferred/depth_only_ps.hlsl");

	// Get input layout
	auto input_layout = egx::MeshVertex::GetInputLayout();

	// Create PSO
	depth_only_ps.SetRootSignature(depth_only_rs);
	depth_only_ps.SetInputLayout(input_layout);
	depth_only_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	depth_only_ps.SetVertexShader(VS);
	depth_only_ps.SetPixelShader(PS);
	depth_only_ps.SetDepthStencilFormat(g_buffer.DepthBuffer().Format());
	depth_only_ps.SetRenderTargetFormat(egx::TextureFormat::UNKNOWN);

	depth_only_ps.SetBlendState(egx::BlendState::NoBlend());
	depth_only_ps.SetRasterState(egx::RasterState::Default());
	depth_only_ps.SetDepthStencilState(egx::DepthStencilState::DepthOn());
	depth_only_ps.Finalize(dev);
}

void DeferredRenderer::initializeModelRenderer(egx::Device& dev, float mipmap_bias)
{
	// Create root signature
	auto normal_sampler = egx::Sampler::LinearWrap();
	normal_sampler.SetMipMapBias(0.0f + mipmap_bias);
	auto taa_sampler = egx::Sampler::LinearWrap();
	taa_sampler.SetMipMapBias(-2.0f + mipmap_bias); // (num_samples = 16)
	auto ssaa_sampler = egx::Sampler::LinearWrap();
	//ssaa_sampler.SetMaxMipMapLOD(0.0f);
	ssaa_sampler.SetMipMapBias(-2.5f + mipmap_bias); // 0.5 * log2(num_samples) (num_samples = 32)

	model_rs.InitConstantBuffer(0); // Camera buffer
	model_rs.InitConstantBuffer(1); // Model buffer
	model_rs.InitConstantBuffer(2); // Material buffer
	model_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // Diffuse
	model_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // Normals
	model_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // Specular
	model_rs.InitDescriptorTable(3, egx::ShaderVisibility::Pixel); // Mask
	model_rs.AddSampler(normal_sampler, 0);
	model_rs.AddSampler(taa_sampler, 1);
	model_rs.AddSampler(ssaa_sampler, 2);
	model_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/deferred/deferred_model_nm_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/deferred/deferred_model_nm_ps.hlsl");

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

void DeferredRenderer::initializeLightRenderer(egx::Device& dev)
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
	VS.CompileVertexShader("../Rendering/shaders/deferred/deferred_light_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/deferred/deferred_light_ps.hlsl");

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

void DeferredRenderer::initializeMotionVectorRenderer(egx::Device& dev)
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
	VS.CompileVertexShader("../Rendering/shaders/deferred/motion_vector_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/deferred/motion_vector_ps.hlsl");

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
	motion_vector_ps.SetDepthStencilState(egx::DepthStencilState::DepthEqual());
	motion_vector_ps.Finalize(dev);
}

void DeferredRenderer::recompileShaders(egx::Device& dev)
{
	dev.WaitForGPU();
	auto input_layout = egx::MeshVertex::GetInputLayout();
	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/deferred/deferred_model_nm_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/deferred/deferred_model_nm_ps.hlsl", macro_list);
	model_ps.SetInputLayout(input_layout);
	model_ps.SetVertexShader(VS);
	model_ps.SetPixelShader(PS);
	model_ps.Finalize(dev);
	recompile_shaders = false;
}