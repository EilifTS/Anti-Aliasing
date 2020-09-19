#pragma once
#include <memory>
#include "io/input_manager.h"

#include "graphics/egx.h"
#include "graphics/camera.h"

#include "aa/fxaa.h"
#include "deferred_rendering/deferred_renderer.h"
#include "io/texture_io.h"

class App
{
public:
	App(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

	void Update(eio::InputManager& im);
	void Render(egx::Device& dev, egx::CommandContext& context, eio::InputManager& im);

private:
	egx::FPCamera camera;
	egx::RenderTarget target;

	eio::TextureLoader texture_loader;
	egx::MaterialManager mat_manager;
	DeferrdRenderer renderer;

	// Models
	std::shared_ptr<egx::Model> sponza_model;
	std::shared_ptr<egx::Model> knight_model;

	FXAA fxaa;
};