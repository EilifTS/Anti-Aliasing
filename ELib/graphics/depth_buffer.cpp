#include "depth_buffer.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::DepthBuffer::DepthBuffer(Device& dev, DepthFormat format, const ema::point2D& size)
	:  dsv(),
	Texture2D(
		dev,
		convertDepthFormat(format),
		size,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		ClearValue::Depth1
	)
{
}

void egx::DepthBuffer::CreateDepthStencilView(Device& dev)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = format;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	dsv = dev.dsv_heap->GetNextHandle();

	dev.device->CreateDepthStencilView(buffer.Get(), &desc, dsv);
}

const D3D12_CPU_DESCRIPTOR_HANDLE& egx::DepthBuffer::getDSV() const
{
	assert(dsv.ptr != 0);
	return dsv;
}