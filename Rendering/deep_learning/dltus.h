#pragma once
#include "graphics/egx.h"
#include "graphics/camera.h"

#include "../aa/taa/jitter.h"

#include "master_net.h"

class DLTUS
{
public:
	DLTUS(egx::Device& dev, egx::CommandContext& context, const ema::point2D& window_size, int jitter_count, float upsample_factor);

	const ema::vec2& GetJitter() const { return jitter.Get(jitter_index); };
	const ema::vec2& GetNextJitter() const { return jitter.Get((jitter_index + 1) % jitter_count); };

	void Execute(egx::Device& dev,
		egx::CommandContext& context,
		egx::DepthBuffer& depth_stencil_buffer,
		egx::Texture2D& motion_vectors,
		egx::Texture2D& new_frame,
		egx::RenderTarget& target);
private:
	void initializeRenderInit(egx::Device& dev);
	void initializeRenderFinalize(egx::Device& dev);
	void initializeFormatConverter(egx::Device& dev);

	void renderInitialize(egx::Device& dev,
		egx::CommandContext& context,
		egx::DepthBuffer& depth_stencil_buffer,
		egx::Texture2D& motion_vectors,
		egx::Texture2D& new_frame);
	void renderNetwork(egx::Device& dev, egx::CommandContext& context);
	void renderFinalize(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::Texture2D& new_frame,
		egx::DepthBuffer& depth_stencil_buffer);
	void renderFormatConverter(
		egx::Device& dev,
		egx::CommandContext& context,
		egx::RenderTarget& target);

private:
	egx::MasterNet master_net;

	// Jitter
	Jitter jitter;
	int jitter_count;
	int jitter_index;
	ema::vec2 shader_constants[3];
	ema::point2D window_size;

	// Common shader res
	egx::RenderTarget history_buffer;
	egx::ShaderMacroList macro_list;

	// Initialize render
	egx::RootSignature initialize_rs;
	egx::ComputePipelineState initialize_ps;

	// Finalize render
	egx::RootSignature finalize_rs;
	egx::PipelineState finalize_ps;

	// Format converter
	egx::RootSignature format_converter_rs;
	egx::PipelineState format_converter_ps;
};