#include "ssaa.h"


SSAA::SSAA(egx::Device& dev, const ema::point2D& window_size, int sample_count)
	: jitter(Jitter::Halton(2, 3, sample_count)),
	sample_count(sample_count),
	current_sample_index(0),
	target1(dev, egx::TextureFormat::FLOAT16x4, window_size),
	target2(dev, egx::TextureFormat::FLOAT16x4, window_size)
{
	target1.CreateShaderResourceView(dev);
	target2.CreateShaderResourceView(dev);
	target1.CreateRenderTargetView(dev);
	target2.CreateRenderTargetView(dev);

	initializeSSAA(dev);
	initializeFormatConverter(dev);
}

void SSAA::PrepareForRender(egx::CommandContext& context)
{
	context.SetTransitionBuffer(target1, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(target2, egx::GPUBufferState::RenderTarget);
	context.ClearRenderTarget(target1);
	context.ClearRenderTarget(target2);
	current_sample_index = 0;
}


void SSAA::AddSample(egx::CommandContext& context, egx::Texture2D& new_sample_buffer)
{
	egx::RenderTarget& target = current_sample_index % 2 == 0 ? target1 : target2;
	egx::RenderTarget& prev_target = current_sample_index % 2 == 0 ? target2 : target1;

	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(prev_target, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(new_sample_buffer, egx::GPUBufferState::PixelResource);

	context.SetRootSignature(ssaa_rs);
	context.SetPipelineState(ssaa_ps);

	context.SetRootConstant(0, 1, &current_sample_index);
	context.SetRootDescriptorTable(1, new_sample_buffer);
	context.SetRootDescriptorTable(2, prev_target);

	context.SetRenderTarget(target);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);
	current_sample_index++;
}


void SSAA::Finish(egx::CommandContext& context, egx::RenderTarget& target)
{
	egx::RenderTarget& prev_target = current_sample_index % 2 == 0 ? target2 : target1;

	context.SetTransitionBuffer(prev_target, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);

	context.SetRootSignature(format_converter_rs);
	context.SetPipelineState(format_converter_ps);

	context.SetRootDescriptorTable(0, prev_target);
	context.SetRenderTarget(target);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);
}

void SSAA::initializeSSAA(egx::Device& dev)
{
	// Create root signature
	ssaa_rs.InitConstants(1, 0); // Current index
	ssaa_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // New sample buffer
	ssaa_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // Accumulated sample buffer
	ssaa_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/ssaa/ssaa_vs.hlsl");
	PS.CompilePixelShader("shaders/ssaa/ssaa_ps.hlsl");

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	ssaa_ps.SetRootSignature(ssaa_rs);
	ssaa_ps.SetInputLayout(input_layout);
	ssaa_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	ssaa_ps.SetVertexShader(VS);
	ssaa_ps.SetPixelShader(PS);
	ssaa_ps.SetDepthStencilFormat(egx::TextureFormat::D32);
	ssaa_ps.SetRenderTargetFormat(target1.Format());

	ssaa_ps.SetBlendState(egx::BlendState::NoBlend());
	ssaa_ps.SetRasterState(egx::RasterState::Default());
	ssaa_ps.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	ssaa_ps.Finalize(dev);
}
void SSAA::initializeFormatConverter(egx::Device& dev)
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