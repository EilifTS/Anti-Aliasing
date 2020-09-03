#include "texture2d.h"
#include "internal/egx_internal.h"
#include "device.h"

namespace
{
	DXGI_FORMAT convertFormat(egx::TextureFormat format)
	{
		switch (format)
		{
		case egx::TextureFormat::UNORM4x8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		default: throw std::runtime_error("Unsupported texture format");
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	int formatByteSize(egx::TextureFormat format)
	{
		switch (format)
		{
		case egx::TextureFormat::UNORM4x8:
			return 4;
			break;
		default: throw std::runtime_error("Unsupported texture format");
		}
		return 0;
	}

}

egx::Texture2D::Texture2D(Device& dev, TextureFormat format, const ema::point2D& size)
	: size(size), srv(), rtv(), format(format),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		convertFormat(format),
		size.x, size.y, 1,
		formatByteSize(format),
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_NONE
		)
{
	
}

void egx::Texture2D::CreateShaderResourceView(Device& dev, TextureFormat format)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = convertFormat(format);
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;

	srv = dev.buffer_heap->GetNextHandle();

	dev.device->CreateShaderResourceView(buffer.Get(), &desc, srv);
}

void egx::Texture2D::CreateRenderTargetView(Device& dev)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	
	rtv = dev.rtv_heap->GetNextHandle();

	dev.device->CreateRenderTargetView(buffer.Get(), &desc, rtv);
}
