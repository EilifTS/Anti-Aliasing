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
	initializeTAAStatic(dev);
	initializeTAADynamic(dev);
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

void TAA::Apply(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::DepthBuffer& stencil_buffer,
	egx::Texture2D& distance_buffer,
	egx::Texture2D& motion_vectors,
	egx::Texture2D& new_frame,
	egx::RenderTarget& target,
	egx::Camera& camera)
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
	context.SetTransitionBuffer(distance_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(motion_vectors, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(temp_target, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);

	applyStatic(dev, context, stencil_buffer, distance_buffer, new_frame, camera);
	applyDynamic(dev, context, stencil_buffer, distance_buffer, motion_vectors, new_frame, camera);
	applyFormatConversion(dev, context, target);
}

void TAA::applyStatic(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::DepthBuffer& stencil_buffer,
	egx::Texture2D& distance_buffer,
	egx::Texture2D& new_frame,
	egx::Camera& camera)
{
	

	context.SetRootSignature(taa_static_rs);
	context.SetPipelineState(taa_static_ps);

	context.SetDescriptorHeap(*dev.buffer_heap);
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootConstantBuffer(1, taa_buffer);
	context.SetRootDescriptorTable(2, new_frame);
	context.SetRootDescriptorTable(3, history_buffer);
	context.SetRootDescriptorTable(4, distance_buffer);

	context.SetRenderTarget(temp_target, stencil_buffer);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);
	context.SetStencilRefrenceValue(0);

	context.Draw(4);
}

void TAA::applyDynamic(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::DepthBuffer& stencil_buffer,
	egx::Texture2D& distance_buffer,
	egx::Texture2D& motion_vectors,
	egx::Texture2D& new_frame,
	egx::Camera& camera)
{
	context.SetRootSignature(taa_dynamic_rs);
	context.SetPipelineState(taa_dynamic_ps);

	context.SetDescriptorHeap(*dev.buffer_heap);
	context.SetRootConstantBuffer(0, camera.GetBuffer());
	context.SetRootDescriptorTable(1, new_frame);
	context.SetRootDescriptorTable(2, history_buffer);
	context.SetRootDescriptorTable(3, distance_buffer);
	context.SetRootDescriptorTable(4, motion_vectors);

	context.SetRenderTarget(temp_target, stencil_buffer);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);
	context.SetStencilRefrenceValue(1);

	context.Draw(4);
}


void TAA::applyFormatConversion(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::RenderTarget& target)
{
	// Convert format
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

void TAA::initializeTAAStatic(egx::Device& dev)
{
	// Create root signature
	taa_static_rs.InitConstantBuffer(0); // Camera buffer
	taa_static_rs.InitConstantBuffer(1, egx::ShaderVisibility::Pixel); // TAA buffer
	taa_static_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // New sample texture
	taa_static_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // History buffer
	taa_static_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // Depth buffer
	taa_static_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	taa_static_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/taa/taa_static_vs.hlsl");
	PS.CompilePixelShader("shaders/taa/taa_static_ps.hlsl");

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	taa_static_ps.SetRootSignature(taa_static_rs);
	taa_static_ps.SetInputLayout(input_layout);
	taa_static_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	taa_static_ps.SetVertexShader(VS);
	taa_static_ps.SetPixelShader(PS);
	taa_static_ps.SetDepthStencilFormat(egx::TextureFormat::D24_S8);
	taa_static_ps.SetRenderTargetFormat(history_buffer.Format());

	taa_static_ps.SetBlendState(egx::BlendState::NoBlend());
	taa_static_ps.SetRasterState(egx::RasterState::Default());
	taa_static_ps.SetDepthStencilState(egx::DepthStencilState::CompWithRefStencil());
	taa_static_ps.Finalize(dev);
}
void TAA::initializeTAADynamic(egx::Device& dev)
{
	// Create root signature
	taa_dynamic_rs.InitConstantBuffer(0); // Camera buffer
	taa_dynamic_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // New sample texture
	taa_dynamic_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // History buffer
	taa_dynamic_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // Depth buffer
	taa_dynamic_rs.InitDescriptorTable(3, egx::ShaderVisibility::Pixel); // Motion vectors
	taa_dynamic_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	taa_dynamic_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/taa/taa_dynamic_vs.hlsl");
	PS.CompilePixelShader("shaders/taa/taa_dynamic_ps.hlsl");

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	taa_dynamic_ps.SetRootSignature(taa_dynamic_rs);
	taa_dynamic_ps.SetInputLayout(input_layout);
	taa_dynamic_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	taa_dynamic_ps.SetVertexShader(VS);
	taa_dynamic_ps.SetPixelShader(PS);
	taa_dynamic_ps.SetDepthStencilFormat(egx::TextureFormat::D24_S8);
	taa_dynamic_ps.SetRenderTargetFormat(history_buffer.Format());

	taa_dynamic_ps.SetBlendState(egx::BlendState::NoBlend());
	taa_dynamic_ps.SetRasterState(egx::RasterState::Default());
	taa_dynamic_ps.SetDepthStencilState(egx::DepthStencilState::CompWithRefStencil());
	taa_dynamic_ps.Finalize(dev);
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