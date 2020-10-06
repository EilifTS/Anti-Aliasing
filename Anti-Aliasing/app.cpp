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
	ssaa(dev, im.Window().WindowSize(), 256),
	aa_mode(AAMode::TAA),
	render_mode(RenderMode::Rasterizer),
	scene_update_mode(SceneUpdateMode::OnDemand)
{
	initializeInternals(dev);
	initializeAssets(dev, context);
	initializeRayTracing(dev, context, im.Window().WindowSize());
}

void App::Update(eio::InputManager& im)
{
	handleInput(im);

	if (scene_update_mode != SceneUpdateMode::OnDemand || progress_frame == true)
	{
		float time = (float)((double)im.Clock().GetTime() / 1000000.0);

		if (progress_frame)
		{
			virtual_time += 1.0f / 60.0f;
			time = virtual_time;
		}

		updateScene(time);
	}
}
void App::Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im)
{
	context.SetDescriptorHeap(*dev.buffer_heap);
	camera.UpdateBuffer(dev, context);

	// Only update target2 if in Realtime mode or if in demand mode and progress frame is true
	if (scene_update_mode != SceneUpdateMode::OnDemand || progress_frame == true)
	{
		if (aa_mode != AAMode::SSAA)
		{
			if (render_mode == RenderMode::Rasterizer)
				renderRasterizer(dev, context);
			else
				renderRayTracer(dev, context);

			if (aa_mode == AAMode::None)
			{
				context.SetTransitionBuffer(target1, egx::GPUBufferState::CopySource);
				context.SetTransitionBuffer(target2, egx::GPUBufferState::CopyDest);
				context.CopyBuffer(target1, target2);
			}
			if (aa_mode == AAMode::TAA)
				taa.Apply(dev, context, renderer.GetGBuffer().DepthBuffer(), renderer.GetMotionVectors(), target1, target2, camera);
			else if (aa_mode == AAMode::FXAA)
				fxaa.Apply(dev, context, target1, target2);
		}
		else
		{
			ssaa.PrepareForRender(context);
			for (int i = 0; i < ssaa.GetSampleCount(); i++)
			{
				camera.SetJitter(ssaa.GetJitter());
				camera.Update();
				camera.UpdateBuffer(dev, context);
				if (render_mode == RenderMode::Rasterizer)
					renderRasterizer(dev, context);
				else
					renderRayTracer(dev, context);
				ssaa.AddSample(context, target1);
			}
			ssaa.Finish(context, target2);
		}
	}
	

	auto& back_buffer = context.GetCurrentBackBuffer();
	renderer.ApplyToneMapping(dev, context, target2, back_buffer);
	progress_frame = false;
}


void App::initializeInternals(egx::Device& dev)
{
	if (scene_update_mode == SceneUpdateMode::Realtime)
	{
		camera.SetPosition({ 10.0f, 1.0f, 0.0f });
		camera.SetRotation({ 0.0f, 0.0f, -3.141592f * 0.5f });
	}
	else
	{
		camera.SetPosition({ -10.0f, 1.0f, 0.0f });
		camera.SetRotation({ 0.0f, 0.0f, -3.141592f * 0.3f });
	}

	target1.CreateShaderResourceView(dev);
	target1.CreateRenderTargetView(dev);
	target2.CreateShaderResourceView(dev);
	target2.CreateRenderTargetView(dev);
}

void App::initializeAssets(egx::Device& dev, egx::CommandContext& context)
{
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

	
}

void App::initializeRayTracing(egx::Device& dev, egx::CommandContext& context, const ema::point2D& window_size)
{
	// Create acceleration structures
	if (dev.SupportsRayTracing())
	{
		ray_tracer = std::make_shared<RayTracer>(dev, context, window_size);

		for (auto pmesh : sponza_mesh)
		{
			pmesh->BuildAccelerationStructure(dev, context);
		}
		for (auto pmesh : knight_mesh)
		{
			pmesh->BuildAccelerationStructure(dev, context);
		}
		std::vector<std::shared_ptr<egx::Model>> models_vector = { sponza_model, knight_model1, knight_model2, knight_model3 };
		ray_tracer->BuildTLAS(dev, context, models_vector);

		std::vector<std::shared_ptr<egx::Mesh>> mesh_vector(sponza_mesh);
		mesh_vector.insert(std::end(mesh_vector), std::begin(knight_mesh), std::end(knight_mesh));
		ray_tracer->UpdateShaderTable(dev, camera.GetBuffer(), taa.GetJitterBuffer(), mesh_vector);
	}
}

void App::handleInput(eio::InputManager& im)
{
	if(scene_update_mode == SceneUpdateMode::Realtime)
		camera.HandleInput(im);
	else
	{
		if (im.Keyboard().IsKeyReleased(39)) // Right arrow
		{
			progress_frame = true;
		}
	}

	if (im.Keyboard().IsKeyReleased('1'))
	{
		aa_mode = AAMode::None;
		renderer.SetSampler(false);
	}
	
	if (im.Keyboard().IsKeyReleased('2'))
	{
		aa_mode = AAMode::FXAA;
		renderer.SetSampler(false);
	}
	if (im.Keyboard().IsKeyReleased('3'))
	{
		aa_mode = AAMode::TAA;
		renderer.SetSampler(true);
	}
	if (im.Keyboard().IsKeyReleased('4'))
	{
		aa_mode = AAMode::SSAA;
		renderer.SetSampler(false);
	}
	//if (aa_mode == AAMode::FXAA)
	//	fxaa.HandleInput(im);
	//else if (aa_mode == AAMode::TAA)
	//	taa.HandleInput(im);
}
void App::updateScene(float t)
{
	auto jitter = taa.GetNextJitter();

	camera.SetJitter(jitter);
	camera.Update();
	renderer.UpdateLight(camera);

	if (aa_mode == AAMode::TAA)
	{
		taa.Update(
			camera.InvProjectionMatrixNoJitter(),
			camera.InvViewMatrix(),
			camera.LastViewMatrix(),
			camera.LastProjectionMatrixNoJitter());
	}

	float rot = t;
	knight_model1->SetRotation(ema::vec3(0.0f, 0.0f, rot));
	knight_model2->SetPosition(ema::vec3(-4.4f, 0.0f, 0.5f + 0.5f * sinf(10.0f * t)));
}

void App::renderRasterizer(egx::Device& dev, egx::CommandContext& context)
{
	std::vector<std::shared_ptr<egx::Model>> models = { sponza_model, knight_model1, knight_model2, knight_model3 };
	std::vector<std::shared_ptr<egx::Model>> static_models = { sponza_model };
	std::vector<std::shared_ptr<egx::Model>> dynamic_models = { knight_model1, knight_model2, knight_model3 };

	for (auto pmodel : models) pmodel->UpdateBuffer(dev, context);
	renderer.PrepareFrame(dev, context);
	for (auto pmodel : static_models) renderer.RenderModel(dev, context, camera, *pmodel);
	for (auto pmodel : dynamic_models) renderer.RenderModel(dev, context, camera, *pmodel);
	for (auto pmodel : dynamic_models) renderer.RenderMotionVectors(dev, context, camera, *pmodel);
	renderer.RenderLight(dev, context, camera, target1);
	renderer.PrepareFrameEnd();
}
void App::renderRayTracer(egx::Device& dev, egx::CommandContext& context)
{
	std::vector<std::shared_ptr<egx::Model>> models = { sponza_model, knight_model1, knight_model2, knight_model3 };

	ray_tracer->ReBuildTLAS(context, models);
	renderer.PrepareFrame(dev, context);
	renderer.PrepareFrameEnd();
	auto& trace_result = ray_tracer->Trace(dev, context);
	context.SetTransitionBuffer(trace_result, egx::GPUBufferState::CopySource);
	context.SetTransitionBuffer(target1, egx::GPUBufferState::CopyDest);
	context.CopyBuffer(trace_result, target1);
}