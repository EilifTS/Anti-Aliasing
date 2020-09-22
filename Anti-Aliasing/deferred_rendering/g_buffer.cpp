#include "g_buffer.h"

GBuffer::GBuffer(egx::Device& dev, const ema::point2D& size, float far_plane)
	: 
	diffuse_target(dev, egx::TextureFormat::UNORM8x4, size, ema::color::SkyBlue()),
	normal_target(dev, egx::TextureFormat::FLOAT16x4, size, ema::color(0.0f, 0.0f, 0.0f, far_plane)),
	depth_buffer(dev, egx::TextureFormat::D24_S8, size)
{
	diffuse_target.CreateRenderTargetView(dev);
	diffuse_target.CreateShaderResourceView(dev);
	normal_target.CreateRenderTargetView(dev);
	normal_target.CreateShaderResourceView(dev);
	depth_buffer.CreateDepthStencilView(dev);
}