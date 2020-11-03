#pragma once
#include "../taa/jitter.h"
#include "graphics/egx.h"
#include "graphics/camera.h"

class SSAA
{
public:
	SSAA(egx::Device& dev, const ema::point2D& window_size, int sample_count);

	void PrepareForRender(egx::CommandContext& context);
	void AddSample(egx::CommandContext& context, egx::Texture2D& new_sample_buffer);
	void Finish(egx::CommandContext& context, egx::RenderTarget& target);

	int GetSampleCount() const { return sample_count; };
	const ema::vec2 GetJitter() const { return jitter.Get(current_sample_index); };

private:
	Jitter jitter;
	int sample_count;
	int current_sample_index;

	egx::RootSignature ssaa_rs;
	egx::PipelineState ssaa_ps;
	egx::RootSignature format_converter_rs;
	egx::PipelineState format_converter_ps;

	// Ping pong targets
	egx::RenderTarget target1;
	egx::RenderTarget target2;

private:
	void initializeSSAA(egx::Device& dev);
	void initializeFormatConverter(egx::Device& dev);
};