#include "texture2d.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::Texture2D::Texture2D(Device& dev, TextureFormat format, const ema::point2D& size)
	: size(size), srv(), rtv(), format(convertFormat(format)),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		convertFormat(format),
		size.x, size.y, 1,
		formatByteSize(convertFormat(format)),
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_NONE
		)
{
	
}

void egx::Texture2D::CreateShaderResourceView(Device& dev, TextureFormat format)
{
	createShaderResourceView(dev, convertFormat(format));
}

void egx::Texture2D::createShaderResourceView(Device& dev, DXGI_FORMAT format)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = format;
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

void egx::Texture2D::createRenderTargetViewForBB(Device& dev)
{
	rtv = dev.rtv_heap->GetNextHandle();

	dev.device->CreateRenderTargetView(buffer.Get(), nullptr, rtv);
}

egx::Texture2D::Texture2D(ComPtr<ID3D12Resource> buffer)
	: GPUBuffer(buffer), size(), srv(), rtv(), format()
{
	auto desc = buffer->GetDesc();
	element_count = (int)desc.Width * desc.Height * desc.DepthOrArraySize;
	element_size = formatByteSize(desc.Format);

	size = { (int)desc.Width, (int)desc.Height };
	format = desc.Format;
	
}

const D3D12_CPU_DESCRIPTOR_HANDLE& egx::Texture2D::getSRV() const
{
	assert(srv.ptr != 0);
	return srv;
}
const D3D12_CPU_DESCRIPTOR_HANDLE& egx::Texture2D::getRTV() const
{
	assert(rtv.ptr != 0);
	return rtv;
}