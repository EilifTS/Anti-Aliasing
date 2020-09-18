#include "light_manager.h"
#include "graphics/cpu_buffer.h"

namespace
{
	static const ema::vec3 shadow_map_light_dir = ema::vec3(0.15f, -1.0f, 0.15f).GetNormalized();
	static const ema::point2D shadow_map_size = ema::point2D(2048, 2048) * 2;
	static const float shadow_map_near_plane = 1.0;
	static const float shadow_map_far_plane = 10000.0;

	static const int shadow_bias = 1;
	static const float shadow_slope_scale_bias = 0.1f;
	static const float shadow_bias_clamp = 1.0f;

	struct ShadowMapConstBufferType
	{
		ema::mat4 view_to_shadowmap_matrix;
		ema::vec4 light_dir;
	};
}

LightManager::LightManager(egx::Device& dev, egx::CommandContext& context)
	:
	camera(dev, context, ema::vec2(shadow_map_size) * 1.0f, shadow_map_near_plane, shadow_map_far_plane),
	light_dir(),
	view_to_shadowmap_matrix(ema::mat4::Identity()),
	const_buffer(dev, (int)sizeof(ShadowMapConstBufferType)),
	depth_buffer(dev, egx::TextureFormat::D32, shadow_map_size)
{
	camera.SetPosition(-shadow_map_light_dir*5000.0f);
	camera.SetLookAt({ 0.0f, 0.0f, 0.0f });
	camera.SetUp({ 0.0f, 0.0f, 1.0f });
	camera.UpdateViewMatrix();



	depth_buffer.CreateDepthStencilView(dev);
	depth_buffer.CreateShaderResourceView(dev, egx::TextureFormat::FLOAT32x1);


	// Create root signature
	shadow_rs.InitConstantBuffer(0); // Camera
	shadow_rs.InitConstantBuffer(1); // Material
	shadow_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // Material mask texture
	shadow_rs.AddSampler(egx::Sampler::LinearWrap(), 0);
	shadow_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/shadow_map_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/shadow_map_ps.hlsl");

	// Get input layout
	auto input_layout = egx::MeshVertex::GetInputLayout();

	// Create PSO
	shadow_ps.SetRootSignature(shadow_rs);
	shadow_ps.SetInputLayout(input_layout);
	shadow_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	shadow_ps.SetVertexShader(VS);
	shadow_ps.SetPixelShader(PS);
	shadow_ps.SetDepthStencilFormat(depth_buffer.Format());
	shadow_ps.SetRenderTargetFormat(egx::TextureFormat::UNKNOWN);

	shadow_ps.SetBlendState(egx::BlendState::NoBlend());
	shadow_ps.SetRasterState(egx::RasterState::ShadowMap(shadow_bias, shadow_slope_scale_bias, shadow_bias_clamp));
	shadow_ps.SetDepthStencilState(egx::DepthStencilState::DepthOn());
	shadow_ps.Finalize(dev);
}


void LightManager::Update(const egx::Camera& player_camera)
{
	//view_to_shadowmap_matrix = player_camera.ViewMatrix().Inverse().Transpose();
	view_to_shadowmap_matrix = player_camera.ViewMatrix().Inverse() * camera.ViewMatrix() * camera.ProjectionMatrix();
	//view_to_shadowmap_matrix = camera.ProjectionMatrix() * camera.ViewMatrix() * player_camera.ViewMatrix().Inverse();
	view_to_shadowmap_matrix = view_to_shadowmap_matrix.Transpose();
	light_dir = ema::vec4(shadow_map_light_dir, 0.0f) * player_camera.ViewMatrix();
}

void LightManager::RenderToShadowMap(egx::Device& dev, egx::CommandContext& context, egx::ModelList& models)
{
	// Update camera
	camera.UpdateBuffer(dev, context);

	// Update light buffer
	ShadowMapConstBufferType smcbt;
	smcbt.view_to_shadowmap_matrix = view_to_shadowmap_matrix;
	smcbt.light_dir = light_dir;
	egx::CPUBuffer cpu_buffer(&smcbt, (int)sizeof(smcbt));
	context.SetTransitionBuffer(const_buffer, egx::GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, const_buffer);
	context.SetTransitionBuffer(const_buffer, egx::GPUBufferState::ConstantBuffer);


	context.SetTransitionBuffer(depth_buffer, egx::GPUBufferState::DepthWrite);

	context.ClearDepth(depth_buffer, 1.0f);
	context.SetDepthStencilBuffer(depth_buffer);

	// Set root signature and pipeline state
	context.SetRootSignature(shadow_rs);
	context.SetPipelineState(shadow_ps);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());

	// Set scissor and viewport
	context.SetViewport(shadow_map_size);
	context.SetScissor(shadow_map_size);
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

			if (material.HasMaskTexture())
				context.SetRootDescriptorTable(2, material.GetMaskTexture());

			// Draw
			context.DrawIndexed(pmesh->GetIndexBuffer().GetElementCount());
		}
	}

}