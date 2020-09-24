#include "depth_buffer.h"
#include "internal/egx_internal.h"
#include "device.h"

namespace
{
	D3D12_CLEAR_VALUE convertClearValue(DXGI_FORMAT format, float depth_clear, unsigned char stencil_clear)
	{
		return CD3DX12_CLEAR_VALUE(format, depth_clear, stencil_clear);
	}

	
}

egx::DepthBuffer::DepthBuffer(Device& dev, TextureFormat format, const ema::point2D& size, float depth_clear, unsigned char stencil_clear)
	:  dsv(), clear_value(convertClearValue(convertToDepthStencilFormat(format), depth_clear, stencil_clear)),
	Texture2D(
		dev,
		convertFormat(format),
		size,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		convertClearValue(convertToDepthStencilFormat(format), depth_clear, stencil_clear)
	)
{
	assert(isDepthFormat(format));
}

void egx::DepthBuffer::CreateDepthStencilView(Device& dev)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = convertToDepthStencilFormat(format);
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	dsv = dev.dsv_heap->GetNextHandle();

	dev.device->CreateDepthStencilView(buffer.Get(), &desc, dsv);
}

const D3D12_CPU_DESCRIPTOR_HANDLE& egx::DepthBuffer::getDSV() const
{
	assert(dsv.ptr != 0);
	return dsv;
}