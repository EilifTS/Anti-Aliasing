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
	camera(dev, context, ema::vec2(im.Window().WindowSize()), 1.0f, 10000.0f, 3.141592f / 3.0f, 200.0f, 0.001f),
	target(dev, egx::TextureFormat::UNORM8x4, im.Window().WindowSize()),
	model_manager(),
	renderer(dev, context, im.Window().WindowSize()),
	fxaa(dev, im.Window().WindowSize())
{
	camera.SetPosition({ 1000.0f, 100.0f, 0.0f });
	camera.SetRotation({ 0.0f, 0.0f, -3.141592f * 0.5f });

	target.CreateShaderResourceView(dev);
	target.CreateRenderTargetView(dev);

	// Load assets
	model_manager.LoadMesh(dev, context, "models/sponza");
	//model_manager.DisableDiffuseTextures();
	//model_manager.DisableNormalMaps();
	//model_manager.DisableSpecularMaps();
	//model_manager.DisableMaskTextures();
	model_manager.LoadAssets(dev, context, texture_loader);
}

void App::Update(eio::InputManager& im)
{
	camera.Update(im);
	renderer.UpdateLight(camera);
	fxaa.HandleInput(im);
}
void App::Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
{
	camera.UpdateBuffer(dev, context);

	renderer.RenderModels(dev, context, camera, model_manager.GetModels());
	renderer.RenderLight(dev, context, camera, target);
	
	auto& back_buffer = context.GetCurrentBackBuffer();
	fxaa.Apply(dev, context, target, back_buffer);
}