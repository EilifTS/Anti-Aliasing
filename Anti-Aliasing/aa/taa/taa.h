#pragma once
#include "jitter.h"
#include "math/mat4.h"
#include "graphics/egx.h"

class TAA
{
public:
	TAA(egx::Device& dev, const ema::point2D& window_size, int sample_count);

	const ema::vec2& GetJitter() const { return jitter.Get(current_index); };
	void Update() { current_index = (current_index + 1) % sample_count; };
	void Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& new_frame, egx::RenderTarget& target);

private:
	Jitter jitter;
	int sample_count;
	int current_index;

	egx::RootSignature taa_rs;
	egx::PipelineState taa_ps;
	egx::RootSignature format_converter_rs;
	egx::PipelineState format_converter_ps;
	egx::Texture2D history_buffer;
	egx::RenderTarget temp_target;

private:
	void initializeTAA(egx::Device& dev);
	void initializeFormatConverter(egx::Device& dev);
};