#include "dltus.h"
#include "misc/string_helpers.h"

namespace
{
	// Divide and round up
	static int DivUp(int a, int b)
	{
		return (a + b - 1) / b;
	}
}

DLTUS::DLTUS(egx::Device& dev, egx::CommandContext& context, const ema::point2D& window_size, int jitter_count, float upsample_factor)
	: master_net(dev, context, window_size, upsample_factor),
	//jitter(Jitter::Halton(2, 3, 64*64)),
	jitter(Jitter::Custom((int)upsample_factor)),
	jitter_count(jitter_count), jitter_index(0),
	upsample_factor((int)upsample_factor),
	history_buffer(dev, egx::TextureFormat::FLOAT16x4, window_size),
	window_size(window_size),
	formated_input(dev, egx::TextureFormat::UNORM8x4, window_size / upsample_factor)
{
	history_buffer.CreateShaderResourceView(dev);
	history_buffer.CreateRenderTargetView(dev);

	formated_input.CreateShaderResourceView(dev);
	formated_input.CreateRenderTargetView(dev);

	macro_list.SetMacro("UPSAMPLE_FACTOR", emisc::ToString(upsample_factor));

	shader_constants[0] = ema::vec2((float)window_size.x, (float)window_size.y);
	shader_constants[1] = ema::vec2(1.0f / (float)window_size.x, 1.0f / (float)window_size.y);
	shader_constants[2] = ema::vec2(0.0f, 0.0f);

	initializeFormatInput(dev);
	initializeRenderInit(dev);
	initializeRenderFinalize(dev);
	initializeFormatConverter(dev);
}


void DLTUS::initializeFormatInput(egx::Device& dev)
{
	// Create root signature
	format_input_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	format_input_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/deep_learning/format_input_vs.hlsl", macro_list);
	PS.CompilePixelShader("../Rendering/shaders/deep_learning/format_input_ps.hlsl", macro_list);
	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	format_input_ps.SetRootSignature(format_input_rs);
	format_input_ps.SetInputLayout(input_layout);
	format_input_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	format_input_ps.SetVertexShader(VS);
	format_input_ps.SetPixelShader(PS);
	format_input_ps.SetDepthStencilFormat(egx::TextureFormat::D32);
	format_input_ps.SetRenderTargetFormat(formated_input.Format());

	format_input_ps.SetBlendState(egx::BlendState::NoBlend());
	format_input_ps.SetRasterState(egx::RasterState::Default());
	format_input_ps.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	format_input_ps.Finalize(dev);
}
void DLTUS::initializeRenderInit(egx::Device& dev)
{
	// Create root signature
	initialize_rs.InitConstants(6, 0); // Window values and jitter
	initialize_rs.InitDescriptorTable(0, egx::ShaderVisibility::All); // Input texture
	initialize_rs.InitDescriptorTable(1, egx::ShaderVisibility::All); // History buffer
	initialize_rs.InitDescriptorTable(2, egx::ShaderVisibility::All); // Motion vectors
	initialize_rs.InitDescriptorTable(3, egx::ShaderVisibility::All); // Depth buffer
	initialize_rs.InitUnorderedAccessTable(0, 1, egx::ShaderVisibility::All); // Output
	auto sampler = egx::Sampler::LinearClamp();
	auto sampler2 = egx::Sampler::PointClamp();
	sampler.SetVisibility(egx::ShaderVisibility::All);
	sampler2.SetVisibility(egx::ShaderVisibility::All);
	initialize_rs.AddSampler(sampler, 0);
	initialize_rs.AddSampler(sampler2, 1);
	initialize_rs.Finalize(dev);

	// Create Shaders
	egx::Shader CS;
	CS.CompileComputeShader2("../Rendering/shaders/deep_learning/init_network_cs.hlsl", macro_list, dev.Supports16BitFloat());

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	initialize_ps.SetRootSignature(initialize_rs);
	initialize_ps.SetComputeShader(CS);
	initialize_ps.Finalize(dev);
}

void DLTUS::initializeRenderFinalize(egx::Device& dev)
{
	// Create root signature
	finalize_rs.InitConstants(6, 0); // Window values and jitter
	finalize_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // History
	finalize_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // CNN Result
	finalize_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // input texture
	finalize_rs.InitDescriptorTable(3, egx::ShaderVisibility::Pixel); // Depth buffer
	finalize_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	finalize_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader2("../Rendering/shaders/deep_learning/finalize_network_vs.hlsl", macro_list, dev.Supports16BitFloat());
	PS.CompilePixelShader2("../Rendering/shaders/deep_learning/finalize_network_ps.hlsl", macro_list, dev.Supports16BitFloat());

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	finalize_ps.SetRootSignature(finalize_rs);
	finalize_ps.SetInputLayout(input_layout);
	finalize_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	finalize_ps.SetVertexShader(VS);
	finalize_ps.SetPixelShader(PS);
	finalize_ps.SetDepthStencilFormat(egx::TextureFormat::D32);
	finalize_ps.SetRenderTargetFormat(history_buffer.Format());

	finalize_ps.SetBlendState(egx::BlendState::NoBlend());
	finalize_ps.SetRasterState(egx::RasterState::Default());
	finalize_ps.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	finalize_ps.Finalize(dev);
}

void DLTUS::initializeFormatConverter(egx::Device& dev)
{
	// Create root signature
	format_converter_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	format_converter_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	format_converter_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/deep_learning/format_converter_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/deep_learning/format_converter_ps.hlsl");

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

void DLTUS::Execute(egx::Device& dev,
	egx::CommandContext& context,
	egx::DepthBuffer& depth_stencil_buffer,
	egx::Texture2D& motion_vectors,
	egx::Texture2D& new_frame,
	egx::RenderTarget& target)
{
	shader_constants[2] = jitter.Get(jitter_index);

	renderFormatInput(dev, context, new_frame);
	renderInitialize(dev, context, motion_vectors, depth_stencil_buffer);
	renderNetwork(dev, context);
	renderFinalize(dev, context, depth_stencil_buffer);
	renderFormatConverter(dev, context, target);

	jitter_index = (jitter_index + 1) % jitter_count;
}

void DLTUS::renderFormatInput(egx::Device& dev,
	egx::CommandContext& context,
	egx::Texture2D& input_texture)
{
	context.SetTransitionBuffer(input_texture, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(formated_input, egx::GPUBufferState::RenderTarget);

	context.SetRootSignature(format_input_rs);
	context.SetPipelineState(format_input_ps);

	context.SetRootDescriptorTable(0, input_texture);

	context.SetRenderTarget(formated_input);

	context.SetViewport(window_size / upsample_factor);
	context.SetScissor(window_size / upsample_factor);
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);
	context.SetStencilRefrenceValue(0);

	context.Draw(4);
}

void DLTUS::renderInitialize(egx::Device& dev,
	egx::CommandContext& context,
	egx::Texture2D& motion_vectors,
	egx::DepthBuffer& depth_stencil_buffer)
{
	context.SetTransitionBuffer(formated_input, egx::GPUBufferState::NonPixelResource);
	context.SetTransitionBuffer(depth_stencil_buffer, egx::GPUBufferState::NonPixelResource);
	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::NonPixelResource);
	context.SetTransitionBuffer(motion_vectors, egx::GPUBufferState::NonPixelResource);
	context.SetTransitionBuffer(master_net.GetInputBuffer(), egx::GPUBufferState::UnorderedAccess);

	context.SetComputeRootSignature(initialize_rs);
	context.SetComputePipelineState(initialize_ps);

	context.SetComputeRootConstant(0, 6, &shader_constants);
	context.SetComputeRootDescriptorTable(1, formated_input);
	context.SetComputeRootDescriptorTable(2, history_buffer);
	context.SetComputeRootDescriptorTable(3, motion_vectors);
	context.SetComputeRootDescriptorTable(4, depth_stencil_buffer);
	context.SetComputeRootUAVDescriptorTable(5, master_net.GetInputBuffer());

	context.Dispatch(DivUp(window_size.x, 8), DivUp(window_size.y, 8), 1);
	context.SetUABarrier();
}

void DLTUS::renderNetwork(egx::Device& dev, egx::CommandContext& context)
{
	master_net.Execute(dev, context);
}

void DLTUS::renderFinalize(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::DepthBuffer& depth_stencil_buffer)
{
	context.SetTransitionBuffer(formated_input, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(depth_stencil_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(master_net.GetInputBuffer(), egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(master_net.GetOutputBuffer(), egx::GPUBufferState::PixelResource);

	context.SetRootSignature(finalize_rs);
	context.SetPipelineState(finalize_ps);

	context.SetRootConstant(0, 6, &shader_constants);
	context.SetRootDescriptorTable(1, master_net.GetInputBuffer());
	context.SetRootDescriptorTable(2, master_net.GetOutputBuffer());
	context.SetRootDescriptorTable(3, formated_input);
	context.SetRootDescriptorTable(4, depth_stencil_buffer);

	context.SetRenderTarget(history_buffer);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);
	context.SetStencilRefrenceValue(0);

	context.Draw(4);
}


void DLTUS::renderFormatConverter(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::RenderTarget& target)
{
	// Convert format
	//context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::CopyDest);
	//context.SetTransitionBuffer(temp_target, egx::GPUBufferState::CopySource);
	//context.CopyBuffer(temp_target, history_buffer);

	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);
	context.SetRootSignature(format_converter_rs);
	context.SetPipelineState(format_converter_ps);

	context.SetRootDescriptorTable(0, history_buffer);

	context.SetRenderTarget(target);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);
}