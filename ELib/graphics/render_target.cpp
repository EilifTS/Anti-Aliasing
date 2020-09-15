#include "render_target.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::RenderTarget::RenderTarget(Device& dev, TextureFormat format, const ema::point2D& size, ClearValue clear_value)
	: rtv(),
	Texture2D(
		dev,
		convertFormat(format),
		size,
		D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
		clear_value
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
	: Texture2D(buffer, D3D12_RESOURCE_STATE_PRESENT), rtv()
{
	
}


const D3D12_CPU_DESCRIPTOR_HANDLE& egx::RenderTarget::getRTV() const
{
	assert(rtv.ptr != 0);
	return rtv;
}
