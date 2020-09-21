#include "tone_mapper.h"

ToneMapper::ToneMapper(egx::Device& dev)
{
	// Create root signature
	root_sig.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	root_sig.AddSampler(egx::Sampler::LinearClamp(), 0);
	root_sig.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/deferred/tone_map_vs.hlsl");
	PS.CompilePixelShader("shaders/deferred/tone_map_ps.hlsl");

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	pipe_state.SetRootSignature(root_sig);
	pipe_state.SetInputLayout(input_layout);
	pipe_state.SetPrimitiveTopology(egx::TopologyType::Triangle);
	pipe_state.SetVertexShader(VS);
	pipe_state.SetPixelShader(PS);
	pipe_state.SetDepthStencilFormat(egx::TextureFormat::D32);
	pipe_state.SetRenderTargetFormat(egx::TextureFormat::UNORM8x4SRGB);

	pipe_state.SetBlendState(egx::BlendState::NoBlend());
	pipe_state.SetRasterState(egx::RasterState::Default());
	pipe_state.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	pipe_state.Finalize(dev);
}

void ToneMapper::Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, egx::RenderTarget& target)
{
	context.SetTransitionBuffer(texture, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);
	context.SetRootSignature(root_sig);
	context.SetPipelineState(pipe_state);

	context.SetDescriptorHeap(*dev.buffer_heap);
	context.SetRootDescriptorTable(0, texture);

	context.SetRenderTarget(target);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);
}