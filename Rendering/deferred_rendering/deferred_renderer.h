#pragma once
#include "graphics/egx.h"
#include "g_buffer.h"
#include "graphics/camera.h"
#include "light_manager.h"
#include "graphics/model.h"
#include "tone_mapper.h"

class DeferredRenderer
{
public:
	DeferredRenderer(egx::Device& dev, egx::CommandContext& context, const ema::point2D& size, float far_plane);

	void UpdateLight(egx::Camera& camera);

	void PrepareFrame(egx::Device& dev, egx::CommandContext& context);
	void RenderDepthOnly(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model);
	void RenderModel(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model);
	void RenderLight(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::RenderTarget& target);
	void RenderMotionVectors(egx::Device& dev, egx::CommandContext& context, egx::Camera& camera, egx::Model& model);
	void PrepareFrameEnd() { light_manager.PrepareFrameEnd(); };

	void ApplyToneMapping(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, egx::RenderTarget& target) { tone_mapper.Apply(dev, context, texture, target); };

	GBuffer& GetGBuffer() { return g_buffer; };
	egx::Texture2D& GetMotionVectors() { return motion_vectors; };


	enum class TextureSampler
	{
		SSAABias, TAABias, NoBias
	};
	void SetSampler(TextureSampler sampler);

private:
	GBuffer g_buffer;
	LightManager light_manager;
	ema::point2D size;

	egx::RootSignature depth_only_rs;
	egx::PipelineState depth_only_ps;

	egx::RootSignature model_rs;
	egx::PipelineState model_ps;

	egx::RootSignature light_rs;
	egx::PipelineState light_ps;

	egx::RootSignature motion_vector_rs;
	egx::PipelineState motion_vector_ps;
	egx::RenderTarget motion_vectors;

	ToneMapper tone_mapper;

	// Shader macros
	egx::ShaderMacroList macro_list;
	bool recompile_shaders;

private:
	void initializeDepthOnlyRenderer(egx::Device& dev);
	void initializeModelRenderer(egx::Device& dev);
	void initializeLightRenderer(egx::Device& dev);
	void initializeMotionVectorRenderer(egx::Device& dev);

	void recompileShaders(egx::Device& dev);
};