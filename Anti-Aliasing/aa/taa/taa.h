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

	void Apply(
		egx::Device& dev, 
		egx::CommandContext& context, 
		egx::DepthBuffer& stencil_buffer, 
		egx::Texture2D& distance_buffer, 
		egx::Texture2D& motion_vectors, 
		egx::Texture2D& new_frame, 
		egx::RenderTarget& target, 
		egx::Camera& camera);
	
private:
	Jitter jitter;
	int sample_count;
	int current_index;

	egx::RootSignature taa_static_rs;
	egx::PipelineState taa_static_ps;
	egx::RootSignature taa_dynamic_rs;
	egx::PipelineState taa_dynamic_ps;
	egx::RootSignature rectification_rs;
	egx::PipelineState rectification_ps;
	egx::RootSignature format_converter_rs;
	egx::PipelineState format_converter_ps;

	egx::Texture2D history_buffer;
	egx::RenderTarget reprojected_history;
	egx::RenderTarget temp_target;

	egx::ConstantBuffer taa_buffer;
	ema::mat4 view_to_prev_clip;
	ema::vec4 window_size;

private:
	void applyStatic(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::DepthBuffer& stencil_buffer,
		egx::Texture2D& distance_buffer,
		egx::Camera& camera);
	void applyDynamic(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::DepthBuffer& stencil_buffer,
		egx::Texture2D& distance_buffer,
		egx::Texture2D& motion_vectors);

	void applyRectification(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::Texture2D& new_frame);

	void applyFormatConversion(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::RenderTarget& target,
		egx::Texture2D& motion_vectors);

	void initializeTAAStatic(egx::Device& dev);
	void initializeTAADynamic(egx::Device& dev);
	void initializeRectification(egx::Device& dev);
	void initializeFormatConverter(egx::Device& dev);
};