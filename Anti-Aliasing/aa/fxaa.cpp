#include "fxaa.h"

FXAA::FXAA(egx::Device& dev, const ema::point2D& window_size)
	: reciprocal_window_size(1.0f / (float)window_size.x, 1.0f / (float)window_size.y),
	root_sig(), pso()
{
	// Create root signature
	root_sig.InitConstants(2, 0, egx::ShaderVisibility::Pixel);
	root_sig.InitDescriptorTable(0, egx::ShaderVisibility::Pixel);
	root_sig.AddSampler(egx::Sampler::LinearClamp(), 0);
	root_sig.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/fxaa/fxaa_vs.hlsl", macro_list);
	PS.CompilePixelShader("shaders/fxaa/fxaa_ps.hlsl", macro_list);
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
	pso.SetRenderTargetFormat(egx::TextureFormat::UNORM8x4SRGB);

	pso.SetBlendState(egx::BlendState::NoBlend());
	pso.SetRasterState(egx::RasterState::Default());
	pso.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	pso.Finalize(dev);
}

void FXAA::HandleInput(const eio::InputManager& im)
{
	auto& keyboard = im.Keyboard();

	std::string macro_to_set = "";

	if (keyboard.IsKeyReleased('1')) macro_to_set = "DEBUG_NO_FXAA";
	if (keyboard.IsKeyReleased('2')) macro_to_set = "DEBUG_SHOW_EDGES";
	if (keyboard.IsKeyReleased('3')) macro_to_set = "DEBUG_SHOW_EDGE_DIRECTION";
	if (keyboard.IsKeyReleased('4')) macro_to_set = "DEBUG_SHOW_EDGE_SIDE";
	if (keyboard.IsKeyReleased('5')) macro_to_set = "DEBUG_SHOW_CLOSEST_END";
	if (keyboard.IsKeyReleased('6')) macro_to_set = "DEBUG_NO_SUBPIXEL_ANTIALIASING";
	if (keyboard.IsKeyReleased('7'))
	{
		macro_list.Clear();
		recompile_shaders = true;
	}
	
	if (macro_to_set != "")
	{
		macro_list.Clear();
		macro_list.SetMacro(macro_to_set, "1");
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

	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleStrip);

	context.Draw(4);
}

void FXAA::recompileShaders(egx::Device& dev)
{
	// Recompile shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/fxaa/fxaa_vs.hlsl", macro_list);
	PS.CompilePixelShader("shaders/fxaa/fxaa_ps.hlsl", macro_list);
	recompile_shaders = false;

	// Update PSO
	pso.SetVertexShader(VS);
	pso.SetPixelShader(PS);
	pso.Finalize(dev);
}