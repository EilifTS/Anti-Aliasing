#pragma once
#include "math/point2d.h"
#include "graphics/egx.h"
#include "io/input_manager.h"

class FXAA
{
public:
	FXAA(egx::Device& dev, const ema::point2D& window_size);

	void HandleInput(const eio::InputManager& im);

	void Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, egx::RenderTarget& target);

private:
	void recompileShaders(egx::Device& dev);

private:
	bool recompile_shaders;

	ema::point2D window_size;
	ema::vec2 reciprocal_window_size;
	egx::RootSignature root_sig;
	egx::PipelineState pso;


	egx::ShaderMacroList macro_list;
	int debug_mode_index;
	int tuning_edge_threshold_index;
	int tuning_edge_threshold_min_index;
};