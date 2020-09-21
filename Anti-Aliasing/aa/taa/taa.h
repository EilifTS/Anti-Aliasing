#pragma once
#include "jitter.h"
#include "math/mat4.h"
#include "graphics/egx.h"
#include "graphics/camera.h"

class TAA
{
public:
	TAA(egx::Device& dev, const ema::point2D& window_size, int sample_count);

	const ema::vec2& GetJitter() const { return jitter.Get(current_index); };
	const ema::vec2& GetNextJitter() const { return jitter.Get((current_index + 1) % sample_count); };
	void Update(const ema::mat4& prev_frame_view_matrix, const ema::mat4& prev_frame_proj_matrix_no_jitter, const ema::mat4& inv_view_matrix);
	void Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& new_frame, egx::Texture2D& depth_buffer, egx::RenderTarget& target, egx::Camera& camera);

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

	egx::ConstantBuffer taa_buffer;
	ema::mat4 view_to_prev_clip;

private:
	void initializeTAA(egx::Device& dev);
	void initializeFormatConverter(egx::Device& dev);
};