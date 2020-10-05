#pragma once
#include "jitter.h"
#include "math/mat4.h"
#include "graphics/egx.h"
#include "graphics/camera.h"

class TAA
{
public:
	TAA(egx::Device& dev, const ema::point2D& window_size, int sample_count);

	egx::ConstantBuffer& GetJitterBuffer() { return jitter_buffer; };

	const ema::vec2& GetJitter() const { return jitter.Get(current_index); };
	const ema::vec2& GetNextJitter() const { return jitter.Get((current_index + 1) % sample_count); };
	void Update(
		const ema::mat4& inv_proj_matrix_no_jitter,
		const ema::mat4& inv_view_matrix,
		const ema::mat4& prev_frame_view_matrix,
		const ema::mat4& prev_frame_proj_matrix_no_jitter);

	void HandleInput(const eio::InputManager& im);

	void Apply(
		egx::Device& dev, 
		egx::CommandContext& context, 
		egx::DepthBuffer& depth_stencil_buffer, 
		egx::Texture2D& motion_vectors, 
		egx::Texture2D& new_frame, 
		egx::RenderTarget& target, 
		egx::Camera& camera);
	
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
	ema::mat4 clip_to_prev_clip;
	ema::vec4 window_size;

	// Ray tracing
	static const int max_jitters = 128;
	struct jitterBufferType
	{
		int jitter_count;
		int sample_count;
		int current_index;
		int placeholder;
		ema::vec4 jitters[max_jitters];
	};
	jitterBufferType jbt;
	egx::ConstantBuffer jitter_buffer;

	// Shader macros
	egx::ShaderMacroList macro_list;
	bool recompile_shaders = false;
	int  taa_preset = 3;
	bool taa_use_catmul_room = true;
	bool taa_use_history_rectification = true;
	bool taa_use_ycocg = true;
	bool taa_use_clipping = true;

private:
	void applyTAA(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::DepthBuffer& depth_stencil_buffer,
		egx::Texture2D& motion_vectors,
		egx::Texture2D& new_frame,
		egx::Camera& camera);

	void applyFormatConversion(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::RenderTarget& target,
		egx::Texture2D& motion_vectors);

	void initializeTAA(egx::Device& dev);
	void initializeFormatConverter(egx::Device& dev);

	void recompileShaders(egx::Device& dev);

	void updateJitterBuffer(egx::Device& dev, egx::CommandContext& context);
};