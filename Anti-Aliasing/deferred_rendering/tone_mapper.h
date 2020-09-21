#pragma once
#include "graphics/egx.h"

class ToneMapper
{
public:
	ToneMapper(egx::Device& dev);

	void Apply(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, egx::RenderTarget& target);

private:

	egx::RootSignature root_sig;
	egx::PipelineState pipe_state;
};