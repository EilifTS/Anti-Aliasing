#include "fxaa.h"
#include "io/console.h"
#include "misc/string_helpers.h"

FXAA::FXAA(egx::Device& dev, const ema::point2D& window_size)
	: window_size(window_size),
	reciprocal_window_size(1.0f / (float)window_size.x, 1.0f / (float)window_size.y),
	root_sig(), pso(),
	debug_mode_index(6), tuning_edge_threshold_index(2), tuning_edge_threshold_min_index(1)
{
	// Create root signature
	root_sig.InitConstants(2, 0, egx::ShaderVisibility::Pixel);
	root_sig.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	root_sig.AddSampler(egx::Sampler::LinearClamp(), 0);
	root_sig.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/fxaa/fxaa_vs.hlsl", macro_list);
	PS.CompilePixelShader("../Rendering/shaders/fxaa/fxaa_ps.hlsl", macro_list);
	recompile_shaders = false;

	// Empty nput layout
	egx::InputLayout input_layout;

	// Create PSO
	pso.SetRootSignature(root_sig);
	pso.SetInputLayout(input_layout);
	pso.SetPrimitiveTopology(egx::TopologyType::Triangle);
	pso.SetVertexShader(VS);
	pso.SetPixelShader(PS);
	pso.SetDepthStencilFormat(egx::TextureFormat::D32);
	pso.SetRenderTargetFormat(egx::TextureFormat::UNORM8x4);

	pso.SetBlendState(egx::BlendState::NoBlend());
	pso.SetRasterState(egx::RasterState::Default());
	pso.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	pso.Finalize(dev);
}

void FXAA::HandleInput(const eio::InputManager& im)
{
	auto& keyboard = im.Keyboard();

	const static std::string debug_values[] =
	{
		"DEBUG_NO_FXAA",
		"DEBUG_SHOW_EDGES",
		"DEBUG_SHOW_EDGE_DIRECTION",
		"DEBUG_SHOW_EDGE_SIDE",
		"DEBUG_SHOW_CLOSEST_END",
		"DEBUG_NO_SUBPIXEL_ANTIALIASING"
	};

	int new_debug_mode_index = debug_mode_index;
	if (keyboard.IsKeyReleased('1')) new_debug_mode_index = 0;
	if (keyboard.IsKeyReleased('2')) new_debug_mode_index = 1;
	if (keyboard.IsKeyReleased('3')) new_debug_mode_index = 2;
	if (keyboard.IsKeyReleased('4')) new_debug_mode_index = 3;
	if (keyboard.IsKeyReleased('5')) new_debug_mode_index = 4;
	if (keyboard.IsKeyReleased('6')) new_debug_mode_index = 5;
	if (keyboard.IsKeyReleased('7')) new_debug_mode_index = 6;
	if (new_debug_mode_index != debug_mode_index)
	{
		if(debug_mode_index != 6)
			macro_list.RemoveMacro(debug_values[debug_mode_index]);
		debug_mode_index = new_debug_mode_index;
		if(new_debug_mode_index != 6)
			macro_list.SetMacro(debug_values[debug_mode_index], "1");
		recompile_shaders = true;
	}
	
	
	const static std::string threshold_values[] = 
	{ 
		"1.0 / 3.0",	// Too little
		"1.0 / 4.0",	// Low quality
		"1.0 / 8.0",	// High quality
		"1.0 / 16.0",	// Overkill
	};
	const static std::string threshold_min_values[] =
	{
		"1.0 / 32.0",	// Visible limit
		"1.0 / 16.0",	// High quality
		"1.0 / 12.0",	// Upper limit
	};
	if (keyboard.IsKeyReleased('E'))
	{
		tuning_edge_threshold_index = (tuning_edge_threshold_index + 1) % (int)_countof(threshold_values);
		macro_list.SetMacro("EDGE_THRESHOLD", threshold_values[tuning_edge_threshold_index]);
		recompile_shaders = true;
	}
	if (keyboard.IsKeyReleased('R'))
	{
		tuning_edge_threshold_min_index = (tuning_edge_threshold_min_index + 1) % (int)_countof(threshold_min_values);
		macro_list.SetMacro("EDGE_THRESHOLD_MIN", threshold_min_values[tuning_edge_threshold_min_index]);
		recompile_shaders = true;
	}

	
}

void FXAA::Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, egx::RenderTarget& target)
{
	if (recompile_shaders) recompileShaders(dev);

	context.SetTransitionBuffer(texture, egx::GPUBufferState::PixelResource);
	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);
	context.SetRootSignature(root_sig);
	context.SetPipelineState(pso);

	context.SetDescriptorHeap(*dev.buffer_heap);

	context.SetRenderTarget(target);
	context.SetRootConstant(0, 2, &reciprocal_window_size);
	context.SetRootDescriptorTable(1, texture);

	context.SetViewport(window_size);
	context.SetScissor(window_size);
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);
}

void FXAA::recompileShaders(egx::Device& dev)
{
	dev.WaitForGPU();
	// Recompile shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("../Rendering/shaders/fxaa/fxaa_vs.hlsl", macro_list);
	PS.CompilePixelShader("../Rendering/shaders/fxaa/fxaa_ps.hlsl", macro_list);
	recompile_shaders = false;

	// Update PSO
	pso.SetVertexShader(VS);
	pso.SetPixelShader(PS);
	pso.Finalize(dev);
}