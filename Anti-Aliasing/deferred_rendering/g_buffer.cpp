#include "g_buffer.h"

GBuffer::GBuffer(egx::Device& dev, const ema::point2D& size)
	: 
	diffuse_target(dev, egx::TextureFormat::UNORM8x4, size),
	normal_target(dev, egx::TextureFormat::FLOAT16x4, size, egx::ClearValue::Clear0001),
	depth_buffer(dev, egx::TextureFormat::D32, size)
{
	diffuse_target.CreateRenderTargetView(dev);
	diffuse_target.CreateShaderResourceView(dev);
	normal_target.CreateRenderTargetView(dev);
	normal_target.CreateShaderResourceView(dev);
	depth_buffer.CreateDepthStencilView(dev);
}