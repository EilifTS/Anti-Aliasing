#include "unordered_access_buffer.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::UnorderedAccessBuffer::UnorderedAccessBuffer(Device& dev, TextureFormat format, const ema::point2D& size)
	: uav_cpu(), uav_gpu(), srv_cpu(), srv_gpu(), format(format),
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
egx::UnorderedAccessBuffer::UnorderedAccessBuffer(Device& dev, UINT64 size)
	: uav_cpu(), uav_gpu(), srv_cpu(), srv_gpu(), format(format),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_BUFFER,
		DXGI_FORMAT_UNKNOWN,
		(int)size, 1, 1,
		1,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		nullptr, GPUBufferState::UnorderedAccess)
{

}

void egx::UnorderedAccessBuffer::CreateUnorderedAccessView(Device& dev, bool is_buffer)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC desc = {};
	desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	if (is_buffer)
	{
		desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		desc.Format = DXGI_FORMAT_R16_FLOAT;
		desc.Buffer.FirstElement = 0;
		desc.Buffer.NumElements = GetElementCount() / 2; // Hacky
		desc.Buffer.StructureByteStride = 0;
		desc.Buffer.CounterOffsetInBytes = 0;
		desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	}

	uav_cpu = dev.buffer_heap->GetNextHandle();

	dev.device->CreateUnorderedAccessView(buffer.Get(), nullptr, &desc, uav_cpu);
	uav_gpu = dev.buffer_heap->GetGPUHandle(uav_cpu);
}

void egx::UnorderedAccessBuffer::CreateShaderResourceView(Device& dev)
{
	auto buffer_desc = buffer->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = DXGI_FORMAT_R16_FLOAT;
	desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	desc.Texture2D.MipLevels = buffer_desc.MipLevels;
	desc.Buffer.FirstElement = 0;
	desc.Buffer.NumElements = GetElementCount() / 2; // Hacky assuming F16
	desc.Buffer.StructureByteStride = 0;
	desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	srv_cpu = dev.buffer_heap->GetNextHandle();

	dev.device->CreateShaderResourceView(buffer.Get(), &desc, srv_cpu);
	srv_gpu = dev.buffer_heap->GetGPUHandle(srv_cpu);
}
	
