#pragma once
#include <memory>
#include "io/input_manager.h"

#include "graphics/egx.h"
#include "graphics/camera.h"

#include "aa/fxaa/fxaa.h"
#include "aa/taa/taa.h"
#include "aa/ssaa/ssaa.h"
#include "deferred_rendering/deferred_renderer.h"
#include "io/texture_io.h"

#include "ray_tracer/ray_tracer.h"

#include "network/dataset_video_recorder.h"

enum class AAMode
{
	None, FXAA, TAA, SSAA
};

enum class SceneUpdateMode
{
	Realtime, OnDemand
};

enum class RenderMode
{
	Rasterizer, RayTracer
};

class App
{
public:
	App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

	void Update(eio::InputManager& im);
	void Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

private:
	// Internals
	egx::FPCamera camera;
	egx::RenderTarget renderer_target;
	egx::RenderTarget aa_target;
	egx::RenderTarget aa_target_upsampled;

	// Mode
	AAMode aa_mode;
	RenderMode render_mode;
	SceneUpdateMode scene_update_mode;
	bool progress_frame = true;
	float virtual_time = 0.0f;

	// Assets
	eio::TextureLoader texture_loader;
	egx::MaterialManager mat_manager;

	std::vector<std::shared_ptr<egx::Mesh>> sponza_mesh;
	std::vector<std::shared_ptr<egx::Mesh>> knight_mesh;
	std::shared_ptr<egx::Model> sponza_model;
	std::shared_ptr<egx::Model> knight_model1;
	std::shared_ptr<egx::Model> knight_model2;
	std::shared_ptr<egx::Model> knight_model3;

	// Anti aliasing
	FXAA fxaa;
	TAA taa;
	SSAA ssaa;

	// Renderers
	DeferrdRenderer renderer;
	std::shared_ptr<RayTracer> ray_tracer;

	// Network
	enn::DatasetVideoRecorder dataset_recorder;

	// Temp
	bool do_screen_shot = false;
	int ss_nr = 0;

private:
	void initializeInternals(egx::Device& dev);
	void initializeAssets(egx::Device& dev, egx::CommandContext& context);
	void initializeRayTracing(egx::Device& dev, egx::CommandContext& context, const ema::point2D& window_size);

	void handleInput(eio::InputManager& im);
	void updateScene(float t);

	void renderRasterizer(egx::Device& dev, egx::CommandContext& context);
	void renderRayTracer(egx::Device& dev, egx::CommandContext& context);
};