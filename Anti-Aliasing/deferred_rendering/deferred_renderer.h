#pragma once
#include "graphics/egx.h"
#include "g_buffer.h"
#include "graphics/camera.h"
#include "light_manager.h"
#include "graphics/model.h"
#include "tone_mapper.h"

class DeferrdRenderer
{
public:
	DeferrdRenderer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& size, float far_plane);

	void UpdateLight(egx::Camera& camera);

	void PrepareFrame(egx::Device& dev, egx::CommandContext& context);
	void RenderModel(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model);
	void RenderLight(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::RenderTarget& target);
	void PrepareFrameEnd() { light_manager.PrepareFrameEnd(); };

	void ApplyToneMapping(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, egx::RenderTarget& target) { tone_mapper.Apply(dev, context, texture, target); };

	GBuffer& GetGBuffer() { return g_buffer; };

private:
	GBuffer g_buffer;
	LightManager light_manager;

	egx::RootSignature model_rs;
	egx::PipelineState model_ps;

	egx::RootSignature light_rs;
	egx::PipelineState light_ps;

	ToneMapper tone_mapper;

private:
	void initializeModelRenderer(egx::Device& dev, const ema::point2D& size);
	void initializeLightRenderer(egx::Device& dev, const ema::point2D& size);
};