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
	target1(dev, egx::TextureFormat::UNORM8x4, im.Window().WindowSize()),
	target2(dev, egx::TextureFormat::UNORM8x4, im.Window().WindowSize()),
	renderer(dev, context, im.Window().WindowSize()),
	fxaa(dev, im.Window().WindowSize()),
	taa(dev, im.Window().WindowSize(), 1600),
	aa_mode(AAMode::TAA)
{
	camera.SetPosition({ 1000.0f, 100.0f, 0.0f });
	camera.SetRotation({ 0.0f, 0.0f, -3.141592f * 0.5f });

	target1.CreateShaderResourceView(dev);
	target1.CreateRenderTargetView(dev);
	target2.CreateShaderResourceView(dev);
	target2.CreateRenderTargetView(dev);

	// Load assets
	sponza_model = std::make_shared<egx::Model>(dev, eio::LoadMeshFromOBJB(dev, context, "models/sponza", mat_manager));
	sponza_model->SetStatic(true);
	knight_model = std::make_shared<egx::Model>(dev, eio::LoadMeshFromOBJB(dev, context, "models/knight", mat_manager));
	knight_model->SetScale(120.0f);
	//mat_manager.DisableDiffuseTextures();
	//mat_manager.DisableNormalMaps();
	//mat_manager.DisableSpecularMaps();
	//mat_manager.DisableMaskTextures();
	mat_manager.LoadMaterialAssets(dev, context, texture_loader);
}

void App::Update(eio::InputManager& im)
{
	ema::vec2 jitter = ema::vec2(0.5f);
	if (aa_mode == AAMode::TAA)
	{
		taa.Update();
		jitter = taa.GetJitter();
	}

	camera.Update(im, jitter);
	renderer.UpdateLight(camera);

	if(aa_mode == AAMode::FXAA)
		fxaa.HandleInput(im);

	if (im.Keyboard().IsKeyReleased('Q'))
	{
		if (aa_mode == AAMode::FXAA) aa_mode = AAMode::TAA;
		else if (aa_mode == AAMode::TAA) aa_mode = AAMode::FXAA;
	}

	float rot = (float)((double)im.Clock().GetTime() / 1000000.0);
	//if(fmod(rot, 1.0f) < 0.5f)
	//	knight_model->SetRotation(ema::vec3(0.0f, 0.0f, rot));
	//else
	//	knight_model->SetRotation(ema::vec3(0.0f, 0.0f, 0.0f));
	knight_model->SetRotation(ema::vec3(0.0f, 0.0f, rot));
}
void App::Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
{
	camera.UpdateBuffer(dev, context);
	sponza_model->UpdateBuffer(dev, context);
	knight_model->UpdateBuffer(dev, context);

	renderer.PrepareFrame(dev, context);
	renderer.RenderModel(dev, context, camera, *sponza_model);
	renderer.RenderModel(dev, context, camera, *knight_model);
	renderer.RenderLight(dev, context, camera, target1);
	renderer.PrepareFrameEnd();

	if (aa_mode == AAMode::TAA)
		taa.Apply(dev, context, target1, target2);
	else if(aa_mode == AAMode::FXAA)
		fxaa.Apply(dev, context, target1, target2);

	auto& back_buffer = context.GetCurrentBackBuffer();
	renderer.ApplyToneMapping(dev, context, target2, back_buffer);

}