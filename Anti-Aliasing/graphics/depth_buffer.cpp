#include "depth_buffer.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::DepthBuffer::DepthBuffer(Device& dev, DepthFormat format, const ema::point2D& size)
	: size(size), srv(), dsv(), format(convertDepthFormat(format)),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		convertDepthFormat(format),
		size.x, size.y, 1,
		depthFormatByteSize(convertDepthFormat(format)),
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_NONE
	)
{

}

void egx::DepthBuffer::CreateShaderResourceView(Device& dev)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = format;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = 1;

	srv = dev.buffer_heap->GetNextHandle();

	dev.device->CreateShaderResourceView(buffer.Get(), &desc, srv);
}

void egx::DepthBuffer::CreateDepthStencilView(Device& dev)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC desc = {};
	desc.Format = format;
	desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	dsv = dev.dsv_heap->GetNextHandle();

	dev.device->CreateDepthStencilView(buffer.Get(), &desc, dsv);
}

const D3D12_CPU_DESCRIPTOR_HANDLE& egx::DepthBuffer::getSRV() const
{
	assert(srv.ptr != 0);
	return srv;
}
const D3D12_CPU_DESCRIPTOR_HANDLE& egx::DepthBuffer::getDSV() const
{
	assert(dsv.ptr != 0);
	return dsv;
}