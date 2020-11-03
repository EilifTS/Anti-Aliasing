#pragma once
#include "graphics/egx.h"

class GBuffer
{
public:
	GBuffer(egx::Device& dev, const ema::point2D& size, float far_plane);

	inline egx::RenderTarget& DiffuseBuffer() { return diffuse_target; };
	inline egx::RenderTarget& NormalBuffer() { return normal_target; };
	inline egx::DepthBuffer& DepthBuffer() { return depth_buffer; };

private:
	egx::RenderTarget diffuse_target;
	egx::RenderTarget normal_target;
	egx::DepthBuffer depth_buffer;
};