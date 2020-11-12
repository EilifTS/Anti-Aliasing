#include "taa.h"
#include "graphics/cpu_buffer.h"
#include "misc/string_helpers.h"

namespace
{
	struct taaBufferType
	{
		ema::mat4 clip_to_prev_frame_clip;
		ema::vec2 current_jitter;
	};

	static const int sample_count_presets[5]	= {    2,     4,     8,     16,     32 };
}

TAA::TAA(egx::Device& dev, const ema::point2D& window_size, int sample_count, float upsample_factor)
	: jitter(Jitter::Halton(2, 3, sample_count)),
	sample_count(sample_count),
	current_index(0),
	upsample_factor(upsample_factor),
	history_buffer(dev, egx::TextureFormat::FLOAT16x4, window_size),
	temp_target(dev, egx::TextureFormat::FLOAT16x4, window_size),
	taa_buffer(dev, (int)sizeof(taaBufferType)),
	jitter_buffer(dev, (int)sizeof(jitterBufferType)),
	window_size((float)window_size.x, (float)window_size.y, 1.0f / (float)window_size.x, 1.0f / (float)window_size.y)
{
	history_buffer.CreateShaderResourceView(dev);
	temp_target.CreateRenderTargetView(dev);

	macro_list.SetMacro("TAA_UPSAMPLE_FACTOR", emisc::ToString(upsample_factor));
	macro_list.SetMacro("TAA_UPSAMPLE", upsample_factor == 1.0f ? "0" : "1");
	initializeTAA(dev);
	initializeFormatConverter(dev);

	// Initialize jitter buffer
	jbt.jitter_count = max_jitters;
	jbt.sample_count = sample_count;
	Jitter jitter = Jitter::Halton(2, 3, max_jitters);
	for (int i = 0; i < max_jitters; i++)
	{
		auto ji = jitter.Get(i);
		jbt.jitters[i] = ema::vec4(ji.x, ji.y, 0.0f, 0.0f);
	}

}

void TAA::Update(
	const ema::mat4& inv_proj_matrix_no_jitter,
	const ema::mat4& inv_view_matrix,
	const ema::mat4& prev_frame_view_matrix, 
	const ema::mat4& prev_frame_proj_matrix_no_jitter)
{ 
	current_index = (current_index + 1) % sample_count;

	clip_to_prev_clip = (
		inv_proj_matrix_no_jitter *
		(inv_view_matrix *		// Paranthesis important for multiplication precision
		prev_frame_view_matrix) *
		prev_frame_proj_matrix_no_jitter);
};

void TAA::UseRasterizer(bool new_value)
{
	taa_use_rasterizer = new_value;
	recompile_shaders = true;
	macro_list.SetMacro("TAA_USE_RASTERIZER", taa_use_rasterizer ? "1" : "0");
}

void TAA::HandleInput(const eio::InputManager& im)
{
	auto& keyboard = im.Keyboard();

	if (keyboard.IsKeyReleased('E'))
	{
		taa_preset = (taa_preset + 1) % (int)_countof(sample_count_presets);
		sample_count = sample_count_presets[taa_preset];
		jitter = Jitter::Halton(2, 3, sample_count);
		current_index = 0;
		recompile_shaders = true;
	}
	if (keyboard.IsKeyReleased('R'))
	{
		macro_list.SetMacro("TAA_USE_CATMUL_ROM", taa_use_catmul_room ? "0" : "1");
		taa_use_catmul_room = !taa_use_catmul_room;
		recompile_shaders = true;
	}
	if (keyboard.IsKeyReleased('T')) 
	{
		macro_list.SetMacro("TAA_USE_HISTORY_RECTIFICATION", taa_use_history_rectification ? "0" : "1");
		taa_use_history_rectification = !taa_use_history_rectification;
		recompile_shaders = true;
	}
	if (keyboard.IsKeyReleased('Y')) 
	{
		macro_list.SetMacro("TAA_USE_YCOCG", taa_use_ycocg ? "0" : "1");
		taa_use_ycocg = !taa_use_ycocg;
		recompile_shaders = true;
	}
	if (keyboard.IsKeyReleased('U')) 
	{
		macro_list.SetMacro("TAA_USE_CLIPPING", taa_use_clipping ? "0" : "1");
		taa_use_clipping = !taa_use_clipping;
		recompile_shaders = true;
	}
}

void TAA::Apply(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::DepthBuffer& depth_stencil_buffer,
	egx::Texture2D& motion_vectors,
	egx::Texture2D& new_frame,
	egx::RenderTarget& target,
	egx::Camera& camera)
{
	// Recompile if macros are changed
	if (recompile_shaders) recompileShaders(dev);

	// Update constant buffer
	taaBufferType taabt;
	taabt.clip_to_prev_frame_clip = clip_to_prev_clip.Transpose();
	taabt.current_jitter = jitter.Get(current_index);

	egx::CPUBuffer cpu_buffer(&taabt, sizeof(taabt));
	context.SetTransitionBuffer(taa_buffer, egx::GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, taa_buffer);
	context.SetTransitionBuffer(taa_buffer, egx::GPUBufferState::ConstantBuffer);

	updateJitterBuffer(dev, context);

	context.SetTransitionBuffer(depth_stencil_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(new_frame, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(history_buffer, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(motion_vectors, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(temp_target, egx::GPUBufferState::RenderTarget);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);

	applyTAA(dev, context, depth_stencil_buffer, motion_vectors, new_frame, camera);
	applyFormatConversion(dev, context, target);
}

void TAA::applyTAA(
	egx::Device& dev,
	egx::CommandContext& context,
	egx::DepthBuffer& depth_stencil_buffer,
	egx::Texture2D& motion_vectors,
	egx::Texture2D& new_frame,
	egx::Camera& camera)
{
	context.SetRootSignature(taa_rs);
	context.SetPipelineState(taa_ps);

	context.SetRootConstant(0, 4, &window_size);
	context.SetRootConstantBuffer(1, taa_buffer);
	context.SetRootConstantBuffer(2, jitter_buffer);
	context.SetRootDescriptorTable(3, new_frame);
	context.SetRootDescriptorTable(4, history_buffer);
	context.SetRootDescriptorTable(5, motion_vectors);
	context.SetRootDescriptorTable(6, depth_stencil_buffer);

	context.SetRenderTarget(temp_target);

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);
	context.SetStencilRefrenceValue(0);

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

void TAA::initializeTAA(egx::Device& dev)
{
	// Create root signature
	taa_rs.InitConstants(4, 0); // Window values
	taa_rs.InitConstantBuffer(1, egx::ShaderVisibility::Pixel); // TAA buffer
	taa_rs.InitConstantBuffer(2, egx::ShaderVisibility::Pixel); // Jitter buffer
	taa_rs.InitDescriptorTable(0, egx::ShaderVisibility::Pixel); // New sample texture
	taa_rs.InitDescriptorTable(1, egx::ShaderVisibility::Pixel); // History buffer
	taa_rs.InitDescriptorTable(2, egx::ShaderVisibility::Pixel); // Motion vectors
	taa_rs.InitDescriptorTable(3, egx::ShaderVisibility::Pixel); // Depth buffer
	taa_rs.AddSampler(egx::Sampler::LinearClamp(), 0);
	taa_rs.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/taa/taa_vs.hlsl", macro_list);
	PS.CompilePixelShader("../Rendering/shaders/taa/taa_ps.hlsl", macro_list);

	// Empty input layout
	egx::InputLayout input_layout;

	// Create PSO
	taa_ps.SetRootSignature(taa_rs);
	taa_ps.SetInputLayout(input_layout);
	taa_ps.SetPrimitiveTopology(egx::TopologyType::Triangle);
	taa_ps.SetVertexShader(VS);
	taa_ps.SetPixelShader(PS);
	taa_ps.SetDepthStencilFormat(egx::TextureFormat::D32);
	taa_ps.SetRenderTargetFormat(temp_target.Format());

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
	VS.CompileVertexShader("../Rendering/shaders/taa/format_converter_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/taa/format_converter_ps.hlsl");

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


void TAA::recompileShaders(egx::Device& dev)
{
	dev.WaitForGPU();
	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/taa/taa_vs.hlsl");
	PS.CompilePixelShader("../Rendering/shaders/taa/taa_ps.hlsl", macro_list); 
	taa_ps.SetVertexShader(VS);
	taa_ps.SetPixelShader(PS);
	taa_ps.Finalize(dev);
	recompile_shaders = false;
}


void TAA::updateJitterBuffer(egx::Device& dev, egx::CommandContext& context)
{
	jbt.current_index = (current_index + 1) % sample_count; // Buffer is updated after the actual rendering
	egx::CPUBuffer cpu_buffer(&jbt, sizeof(jbt));
	context.SetTransitionBuffer(jitter_buffer, egx::GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, jitter_buffer);
	context.SetTransitionBuffer(jitter_buffer, egx::GPUBufferState::ConstantBuffer);
}