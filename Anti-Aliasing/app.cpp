#include "app.h"
#include "math/vec3.h"
#include "graphics/shader.h"
#include "graphics/input_layout.h"
#include "graphics/cpu_buffer.h"
#include "io/mesh_io.h"
#include "graphics/mesh.h"

namespace
{
	const static float near_plane = 0.1f;
	const static float far_plane = 1000.0f;
}

App::App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
	: 
	camera(dev, context, ema::vec2(im.Window().WindowSize()), near_plane, far_plane, 3.141592f / 3.0f, 2.0f, 0.001f),
	target1(dev, egx::TextureFormat::UNORM8x4, im.Window().WindowSize()),
	target2(dev, egx::TextureFormat::UNORM8x4, im.Window().WindowSize()),
	renderer(dev, context, im.Window().WindowSize(), far_plane),
	fxaa(dev, im.Window().WindowSize()),
	taa(dev, im.Window().WindowSize(), 16),
	aa_mode(AAMode::TAA),
	ray_tracer(dev, context, im.Window().WindowSize())
{
	camera.SetPosition({ 10.0f, 1.0f, 0.0f });
	camera.SetRotation({ 0.0f, 0.0f, -3.141592f * 0.5f });
	//camera.SetPosition({ 700.0f, 300.0f, 0.0f });
	//camera.SetRotation({ 0.0f, 1.3f, -3.141592f * 0.5f });

	target1.CreateShaderResourceView(dev);
	target1.CreateRenderTargetView(dev);
	target2.CreateShaderResourceView(dev);
	target2.CreateRenderTargetView(dev);

	// Load assets
	sponza_mesh = eio::LoadMeshFromOBJB(dev, context, "models/sponza", mat_manager);
	sponza_model = std::make_shared<egx::Model>(dev, sponza_mesh);
	sponza_model->SetScale(0.01f);
	sponza_model->SetStatic(true);

	knight_mesh = eio::LoadMeshFromOBJB(dev, context, "models/knight", mat_manager);
	knight_model1 = std::make_shared<egx::Model>(dev, knight_mesh);
	knight_model2 = std::make_shared<egx::Model>(dev, knight_mesh);
	knight_model3 = std::make_shared<egx::Model>(dev, knight_mesh);
	knight_model1->SetScale(1.2f);
	knight_model2->SetScale(1.2f);
	knight_model3->SetScale(1.2f);
	knight_model1->SetPosition(ema::vec3(-0.8f, 0.0f, 0.5f));
	knight_model2->SetPosition(ema::vec3(-4.4f, 0.0f, 0.5f));
	knight_model3->SetPosition(ema::vec3(3.0f, 0.0f, 0.5f));
	knight_model1->SetRotation(ema::vec3(0.0f, 0.0f, 0.0f));
	knight_model2->SetRotation(ema::vec3(0.0f, 0.0f, 3.141692f));
	knight_model3->SetRotation(ema::vec3(0.0f, 0.0f, 3.141692f));
	//knight_model3->SetStatic(true);

	//mat_manager.DisableDiffuseTextures();
	//mat_manager.DisableNormalMaps();
	//mat_manager.DisableSpecularMaps();
	mat_manager.DisableMaskTextures();
	mat_manager.LoadMaterialAssets(dev, context, texture_loader);

	// Create acceleration structures
	for (auto pmesh : sponza_mesh)
	{
		pmesh->BuildAccelerationStructure(dev, context);
	}
	for (auto pmesh : knight_mesh)
	{
		pmesh->BuildAccelerationStructure(dev, context);
	}

	std::vector<std::shared_ptr<egx::Model>> models_vector = { sponza_model, knight_model1, knight_model2, knight_model3 };
	ray_tracer.BuildTLAS(dev, context, models_vector);

	std::vector<std::shared_ptr<egx::Mesh>> mesh_vector(sponza_mesh);
	mesh_vector.insert(std::end(mesh_vector), std::begin(knight_mesh), std::end(knight_mesh));
	ray_tracer.UpdateShaderTable(dev, camera.GetBuffer(), mesh_vector);
}

void App::Update(eio::InputManager& im)
{
	if (im.Keyboard().IsKeyReleased('Q'))
	{
		if (aa_mode == AAMode::FXAA)
		{
			aa_mode = AAMode::TAA;
			renderer.SetSampler(true);
		}
		else if (aa_mode == AAMode::TAA)
		{
			aa_mode = AAMode::FXAA;
			renderer.SetSampler(false);
		}
	}

	ema::vec2 jitter = ema::vec2(0.5f);
	//if (aa_mode == AAMode::TAA) jitter = taa.GetNextJitter();
	jitter = taa.GetNextJitter();

	ema::mat4 prev_frame_view_matrix = camera.ViewMatrix();
	ema::mat4 prev_frame_proj_matrix_no_jitter = camera.ProjectionMatrixNoJitter();

	camera.Update(im, jitter);
	renderer.UpdateLight(camera);

	if (aa_mode == AAMode::FXAA)
		fxaa.HandleInput(im);
	else if (aa_mode == AAMode::TAA)
		taa.HandleInput(im);

	if (aa_mode == AAMode::TAA)
	{
		taa.Update(
			camera.ProjectionMatrixNoJitter().Inverse(),
			camera.ViewMatrix().Inverse(),
			prev_frame_view_matrix,
			prev_frame_proj_matrix_no_jitter);
	}
	else
	{
		if(im.Keyboard().IsKeyReleased('Z'))taa.Update(
			camera.ProjectionMatrixNoJitter().Inverse(),
			camera.ViewMatrix().Inverse(),
			prev_frame_view_matrix,
			prev_frame_proj_matrix_no_jitter);
	}

	

	float time = (float)((double)im.Clock().GetTime() / 1000000.0);
	float rot = time;
	knight_model1->SetRotation(ema::vec3(0.0f, 0.0f, rot));
	knight_model2->SetPosition(ema::vec3(-4.4f, 0.0f, 0.5f + 0.5f*sinf(10.0f*time)));
}
void App::Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
{
	std::vector<std::shared_ptr<egx::Model>> models_vector = { sponza_model, knight_model1, knight_model2, knight_model3 };
	ray_tracer.ReBuildTLAS(context, models_vector);

	camera.UpdateBuffer(dev, context);
	sponza_model->UpdateBuffer(dev, context);
	knight_model1->UpdateBuffer(dev, context);
	knight_model2->UpdateBuffer(dev, context);
	knight_model3->UpdateBuffer(dev, context);


	bool use_rasterizer = false;
	if (use_rasterizer)
	{
		renderer.PrepareFrame(dev, context);
		renderer.RenderModel(dev, context, camera, *sponza_model);
		renderer.RenderModel(dev, context, camera, *knight_model3);
		renderer.RenderModel(dev, context, camera, *knight_model1);
		renderer.RenderModel(dev, context, camera, *knight_model2);
		renderer.RenderMotionVectors(dev, context, camera, *knight_model1);
		renderer.RenderMotionVectors(dev, context, camera, *knight_model2);
		renderer.RenderMotionVectors(dev, context, camera, *knight_model3);
		renderer.RenderLight(dev, context, camera, target1);
		renderer.PrepareFrameEnd();

		if (aa_mode == AAMode::TAA)
			taa.Apply(dev, context, renderer.GetGBuffer().DepthBuffer(), renderer.GetMotionVectors(), target1, target2, camera);
		else if (aa_mode == AAMode::FXAA)
			fxaa.Apply(dev, context, target1, target2);
	}
	else
	{
		auto& trace_result = ray_tracer.Trace(dev, context);
		context.SetTransitionBuffer(trace_result, egx::GPUBufferState::CopySource);
		context.SetTransitionBuffer(target2, egx::GPUBufferState::CopyDest);
		context.CopyBuffer(trace_result, target2);
	}

	auto& back_buffer = context.GetCurrentBackBuffer();
	renderer.ApplyToneMapping(dev, context, target2, back_buffer);

}