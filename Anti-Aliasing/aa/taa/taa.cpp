#include "taa.h"
#include "graphics/cpu_buffer.h"

namespace
{
	struct taaBufferType
	{
		ema::mat4 view_to_prev_frame_clip;
	};
}

TAA::TAA(egx::Device& dev, const ema::point2D& window_size, int sample_count)
	: jitter(Jitter::Halton(2, 3, sample_count)),
	sample_count(sample_count),
	current_index(0),
	history_buffer(dev, egx::TextureFormat::FLOAT16x4, window_size),
	temp_target(dev, egx::TextureFormat::FLOAT16x4, window_size),
	taa_buffer(dev, (int)sizeof(taaBufferType))
{
	history_buffer.CreateShaderResourceView(dev);
	temp_target.CreateRenderTargetView(dev);
	initializeTAA(dev);
	initializeFormatConverter(dev);
}

void TAA::Update(const ema::mat4& prev_frame_view_matrix, const ema::mat4& prev_frame_proj_matrix_no_jitter, const ema::mat4& inv_view_matrix)
{ 
	current_index = (current_index + 1) % sample_count;
	view_to_prev_clip = (
		inv_view_matrix *
		prev_frame_view_matrix *
		prev_frame_proj_matrix_no_jitter);
};

void TAA::Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& new_frame, egx::Texture2D& depth_buffer, egx::RenderTarget& target, egx::Camera& camera)
{
	// Update constant buffer
	taaBufferType taabt;
	taabt.view_to_prev_frame_clip = view_to_prev_clip.Transpose();

	egx::CPUBuffer cpu_buffer(&taabt, sizeof(taabt));
	context.SetTransitionBuffer(taa_buffer, egx::GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, taa_buffer);
	context.SetTransitionBuffer(taa_buffer, egx::GPUBufferState::ConstantBuffer);

	context.SetTransitionBuffer(new_frame, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(depth_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(temp_target, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);

	context.SetRootSignature(taa_rs);
	context.SetPipelineState(taa_ps);

	context.SetDescriptorHeap(*dev.buffer_heap);
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootConstantBuffer(1, taa_buffer);
	context.SetRootDescriptorTable(2, new_frame);
	context.SetRootDescriptorTable(3, history_buffer);
	context.SetRootDescriptorTable(4, depth_buffer);

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
	taa_rs.InitConstantBuffer(0); // Camera buffer
	taa_rs.InitConstantBuffer(1, egx::ShaderVisibility::Pixel); // TAA buffer
	taa_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // New sample texture
	taa_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // History buffer
	taa_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // Depth buffer
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