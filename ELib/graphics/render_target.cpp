#include "render_target.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::RenderTarget::RenderTarget(Device& dev, TextureFormat format, const ema::point2D& size)
	: rtv(),
	Texture2D(
		dev,
		convertFormat(format),
		size,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		ClearValue::ColorBlue
	)
{

}

void egx::RenderTarget::CreateRenderTargetView(Device& dev)
{
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	rtv = dev.rtv_heap->GetNextHandle();

	dev.device->CreateRenderTargetView(buffer.Get(), &desc, rtv);
}

void egx::RenderTarget::createRenderTargetViewForBB(Device& dev)
{
	rtv = dev.rtv_heap->GetNextHandle();
	D3D12_RENDER_TARGET_VIEW_DESC desc = {};
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

	dev.device->CreateRenderTargetView(buffer.Get(), &desc, rtv);
}

egx::RenderTarget::RenderTarget(ComPtr<ID3D12Resource> buffer)
	: Texture2D(buffer), rtv()
{
	auto desc = buffer->GetDesc();
	element_count = (int)desc.Width * desc.Height * desc.DepthOrArraySize;
	element_size = formatByteSize(desc.Format);

	size = { (int)desc.Width, (int)desc.Height };
	format = desc.Format;
}


const D3D12_CPU_DESCRIPTOR_HANDLE& egx::RenderTarget::getRTV() const
{
	assert(rtv.ptr != 0);
	return rtv;
}
