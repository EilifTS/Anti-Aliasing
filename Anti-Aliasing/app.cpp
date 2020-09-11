#include "app.h"
#include "math/vec3.h"
#include "graphics/shader.h"
#include "graphics/input_layout.h"
#include "graphics/cpu_buffer.h"
#include "io/mesh_io.h"

namespace
{
	struct VertexType
	{
		ema::vec3 pos;
		ema::color color;
	};
}

App::App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
	: 
	camera(dev, context, ema::vec2(im.Window().WindowSize()), 0.1f, 1000.0f, 3.141592f / 3.0f, 10.0f, 0.001f),
	mesh(eio::LoadMeshFromOBJ(dev, context, "models/good-well.obj", "models/good-well.mtl")),
	depth_buffer(dev, egx::DepthFormat::D32, im.Window().WindowSize()),
	target(dev, egx::TextureFormat::UNORM4x8, im.Window().WindowSize()),
	fxaa(dev, im.Window().WindowSize())
{
	camera.SetPosition({ 40.0f, 10.0f, 0.0f });
	camera.SetRotation({ 0.0f, 0.0f, -3.141592f * 0.5f });

	target.CreateShaderResourceView(dev);
	target.CreateRenderTargetView(dev);

	depth_buffer.CreateDepthStencilView(dev);
	context.SetTransitionBuffer(depth_buffer, egx::GPUBufferState::DepthWrite);

	// Create root signature
	root_sig.InitConstantBuffer(0);
	root_sig.Finalize(dev);

	// Create Shaders
	egx::Shader VS;
	egx::Shader PS;
	VS.CompileVertexShader("shaders/test_vs.hlsl");
	PS.CompilePixelShader("shaders/test_ps.hlsl");

	// Get input layout
	auto input_layout = egx::MeshVertex::GetInputLayout();

	// Create PSO
	pipeline_state.SetRootSignature(root_sig);
	pipeline_state.SetInputLayout(input_layout);
	pipeline_state.SetPrimitiveTopology(egx::TopologyType::Triangle);
	pipeline_state.SetVertexShader(VS);
	pipeline_state.SetPixelShader(PS);
	pipeline_state.SetDepthStencilFormat(egx::DepthFormat::D32);
	pipeline_state.SetRenderTargetFormat(egx::TextureFormat::UNORM4x8);

	pipeline_state.SetBlendState(egx::BlendState::NoBlend());
	pipeline_state.SetRasterState(egx::RasterState::Default());
	pipeline_state.SetDepthStencilState(egx::DepthStencilState::DepthOn());
	pipeline_state.Finalize(dev);
}

void App::Update(eio::InputManager& im)
{
	camera.Update(im);
	fxaa.HandleInput(im);
}
void App::Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
{
	camera.UpdateBuffer(dev, context);

	context.SetTransitionBuffer(target, egx::GPUBufferState::RenderTarget);
	context.ClearRenderTarget(target, { 0.117f, 0.565f, 1.0f, 1.0f });
	context.ClearDepth(depth_buffer, 1.0f);
	context.SetRenderTarget(target, depth_buffer);

	// Set root sig
	context.SetRootSignature(root_sig);
	// Set PSO
	context.SetPipelineState(pipeline_state);

	// Set root values
	context.SetRootConstantBuffer(0, camera.GetBuffer());

	// Set vertex buffer
	context.SetVertexBuffer(mesh.GetVertexBuffer());
	context.SetIndexBuffer(mesh.GetIndexBuffer());

	// Set scissor and viewport
	context.SetViewport();
	context.SetScissor();
	context.SetPrimitiveTopology(egx::Topology::TriangleList);

	// Draw?
	context.DrawIndexed(mesh.GetIndexBuffer().GetElementCount());

	auto& back_buffer = context.GetCurrentBackBuffer();
	fxaa.Apply(dev, context, target, back_buffer);
}