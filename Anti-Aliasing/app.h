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

	egx::ModelManager model_manager;
	eio::TextureLoader texture_loader;
	DeferrdRenderer renderer;

	FXAA fxaa;
};