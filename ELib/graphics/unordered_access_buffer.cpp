#include "unordered_access_buffer.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::UnorderedAccessBuffer::UnorderedAccessBuffer(Device& dev, TextureFormat format, const ema::point2D& size)
	: uav_cpu(), uav_gpu(), format(format),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		convertFormat(format),
		size.x, size.y, 1,
		1,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		nullptr, GPUBufferState::UnorderedAccess)
{

}
egx::UnorderedAccessBuffer::UnorderedAccessBuffer(Device& dev, UINT size)
	: uav_cpu(), uav_gpu(), format(format),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_BUFFER,
		DXGI_FORMAT_UNKNOWN,
		size, 1, 1,
		1,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		nullptr, GPUBufferState::UnorderedAccess)
{

}

void egx::UnorderedAccessBuffer::CreateUnorderedAccessView(Device& dev)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	uav_cpu = dev.buffer_heap->GetNextHandle();

	dev.device->CreateUnorderedAccessView(buffer.Get(), nullptr, &desc, uav_cpu);
	uav_gpu = dev.buffer_heap->GetGPUHandle(uav_cpu);
}