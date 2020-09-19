#pragma once
#include "graphics/egx.h"
#include "g_buffer.h"
#include "graphics/camera.h"
#include "light_manager.h"
#include "graphics/model.h"

class DeferrdRenderer
{
public:
	DeferrdRenderer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& size);

	void UpdateLight(egx::Camera& camera);

	void PrepareFrame(egx::Device& dev, egx::CommandContext& context);
	void RenderModel(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model);
	void RenderLight(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::RenderTarget& target);

private:
	GBuffer g_buffer;
	LightManager light_manager;

	egx::RootSignature model_rs;
	egx::PipelineState model_ps;

	egx::RootSignature light_rs;
	egx::PipelineState light_ps;

private:
	void initializeModelRenderer(egx::Device& dev, const ema::point2D& size);
	void initializeLightRenderer(egx::Device& dev, const ema::point2D& size);
};