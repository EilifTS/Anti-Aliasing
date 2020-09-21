#pragma once
#include <memory>
#include "io/input_manager.h"

#include "graphics/egx.h"
#include "graphics/camera.h"

#include "aa/fxaa/fxaa.h"
#include "aa/taa/taa.h"
#include "deferred_rendering/deferred_renderer.h"
#include "io/texture_io.h"

enum class AAMode
{
	FXAA, TAA
};

class App
{
public:
	App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

	void Update(eio::InputManager& im);
	void Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

private:
	egx::FPCamera camera;
	egx::RenderTarget target1;
	egx::RenderTarget target2;

	eio::TextureLoader texture_loader;
	egx::MaterialManager mat_manager;
	DeferrdRenderer renderer;

	// Models
	std::shared_ptr<egx::Model> sponza_model;
	std::shared_ptr<egx::Model> knight_model;

	AAMode aa_mode;
	FXAA fxaa;
	TAA taa;
};