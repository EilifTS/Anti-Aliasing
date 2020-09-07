#include "app.h"
#include "math/vec3.h"
#include "graphics/shader.h"
#include "graphics/input_layout.h"
#include "graphics/cpu_buffer.h"

namespace
{
	struct VertexType
	{
		ema::vec3 pos;
		ema::color color;
	};
}

App::App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
	: vertex_buffer(nullptr)
{
	// Create vetex buffer
	VertexType vertices[] = {
		{{0.0f, 1.0f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
		{{1.0f, -1.0f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
		{{-1.0f, -1.0f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
	};

	egx::CPUBuffer cpu_vertex_buffer(vertices, (int)sizeof(vertices));

	vertex_buffer = std::make_unique<egx::VertexBuffer>(dev, (int)sizeof(VertexType), (int)_countof(vertices));

	dev.ScheduleUpload(context, cpu_vertex_buffer, *vertex_buffer);
	context.SetTransitionBuffer(*vertex_buffer, egx::GPUBufferState::VertexBuffer);

	// Create root signature
	root_sig.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/test_vs.hlsl");
	PS.CompilePixelShader("shaders/test_ps.hlsl");

	// Create input layout
	egx::InputLayout layout;
	layout.AddPosition(3);
	layout.AddColor(4);

	// Create PSO
	pipeline_state.SetRootSignature(root_sig);
	pipeline_state.SetInputLayout(layout);
	pipeline_state.SetPrimitiveTopology(egx::TopologyType::Triangle);
	pipeline_state.SetVertexShader(VS);
	pipeline_state.SetPixelShader(PS);
	pipeline_state.SetDepthStencilFormat(egx::DepthFormat::D32);
	pipeline_state.SetRenderTargetFormat(egx::TextureFormat::UNORM4x8);

	pipeline_state.SetBlendState(egx::BlendState::NoBlend());
	pipeline_state.SetRasterState(egx::RasterState::Default());
	pipeline_state.SetDepthStencilState(egx::DepthStencilState::DepthOff());
	pipeline_state.Finalize(dev);
}

void App::Update(eio::InputManager& im)
{

}
void App::Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
{
	auto& render_target = context.GetCurrentBackBuffer();

	context.SetTransitionBuffer(render_target, egx::GPUBufferState::RenderTarget);
	context.ClearRenderTarget(render_target, { 0.0f, 1.0f, 0.0f, 1.0f });
	context.SetRenderTarget(render_target);

	// Set root sig
	context.SetRootSignature(root_sig);
	// Set PSO
	context.SetPipelineState(pipeline_state);
	// Set vertex buffer
	context.SetVertexBuffer(*vertex_buffer);
	// Set scissor and viewport
	context.SetViewport(im.Window().WindowSize());
	context.SetScissor(im.Window().WindowSize());
	context.SetPrimitiveTopology(egx::Topology::TriangleList);

	// Draw?
	context.Draw(vertex_buffer->GetElementCount());
}