#pragma once
#include "graphics/egx.h"
#include "g_buffer.h"
#include "graphics/camera.h"

class DeferrdRenderer
{
public:
	DeferrdRenderer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& size);

	void UpdateLight(egx::Camera& camera);

	void RenderModels(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::ModelList& models);
	void RenderLight(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::RenderTarget& target);

private:
	GBuffer g_buffer;
	
	egx::RootSignature model_rs;
	egx::PipelineState model_ps;

	ema::vec4 light_dir;
	egx::ConstantBuffer light_buffer;
	egx::RootSignature light_rs;
	egx::PipelineState light_ps;

private:
	void initializeModelRenderer(egx::Device& dev, const ema::point2D& size);
	void initializeLightRenderer(egx::Device& dev, const ema::point2D& size);
};