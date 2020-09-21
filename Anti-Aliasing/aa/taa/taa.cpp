#include "taa.h"

TAA::TAA(egx::Device& dev, const ema::point2D& window_size, int sample_count)
	: jitter(Jitter::Halton(2, 3, sample_count)),
	sample_count(sample_count),
	current_index(0),
	history_buffer(dev, egx::TextureFormat::FLOAT16x4, window_size),
	temp_target(dev, egx::TextureFormat::FLOAT16x4, window_size)
{
	history_buffer.CreateShaderResourceView(dev);
	temp_target.CreateRenderTargetView(dev);
	initializeTAA(dev);
	initializeFormatConverter(dev);
}


void TAA::Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& new_frame, egx::RenderTarget& target)
{
	context.SetTransitionBuffer(new_frame, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(temp_target, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);

	context.SetRootSignature(taa_rs);
	context.SetPipelineState(taa_ps);

	context.SetDescriptorHeap(*dev.buffer_heap);
	context.SetRootDescriptorTable(0, new_frame);
	context.SetRootDescriptorTable(1, history_buffer);

	context.SetRenderTarget(temp_target);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);

	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::CopyDest);
	context.SetTransitionBuffer(temp_target, egx::GPUBufferState::CopySource);
	context.CopyBuffer(temp_target, history_buffer);

	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::PixelResource);
	context.SetRootSignature(format_converter_rs);
	context.SetPipelineState(format_converter_ps);

	context.SetRootDescriptorTable(0, history_buffer);

	context.SetRenderTarget(target);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);
}


void TAA::initializeTAA(egx::Device& dev)
{
	// Create root signature
	taa_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	taa_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel);
	taa_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	taa_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/taa/taa_vs.hlsl");
	PS.CompilePixelShader("shaders/taa/taa_ps.hlsl");

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	taa_ps.SetRootSignature(taa_rs);
	taa_ps.SetInputLayout(input_layout);
	taa_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	taa_ps.SetVertexShader(VS);
	taa_ps.SetPixelShader(PS);
	taa_ps.SetDepthStencilFormat(egx::TextureFormat::D32);
	taa_ps.SetRenderTargetFormat(history_buffer.Format());

	taa_ps.SetBlendState(egx::BlendState::NoBlend());
	taa_ps.SetRasterState(egx::RasterState::Default());
	taa_ps.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	taa_ps.Finalize(dev);
}
void TAA::initializeFormatConverter(egx::Device& dev)
{
	// Create root signature
	format_converter_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	format_converter_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	format_converter_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/taa/format_converter_vs.hlsl");
	PS.CompilePixelShader("shaders/taa/format_converter_ps.hlsl");

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	format_converter_ps.SetRootSignature(format_converter_rs);
	format_converter_ps.SetInputLayout(input_layout);
	format_converter_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	format_converter_ps.SetVertexShader(VS);
	format_converter_ps.SetPixelShader(PS);
	format_converter_ps.SetDepthStencilFormat(egx::TextureFormat::D32);
	format_converter_ps.SetRenderTargetFormat(egx::TextureFormat::UNORM8x4);

	format_converter_ps.SetBlendState(egx::BlendState::NoBlend());
	format_converter_ps.SetRasterState(egx::RasterState::Default());
	format_converter_ps.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	format_converter_ps.Finalize(dev);
}