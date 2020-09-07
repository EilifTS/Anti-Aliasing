#include "constant_buffer.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::ConstantBuffer::ConstantBuffer(Device& dev, int buffer_size)
	: GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_BUFFER,
		DXGI_FORMAT_UNKNOWN,
		buffer_size, 1, 1, 1,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_NONE), cbv()
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = buffer->GetGPUVirtualAddress();
	desc.SizeInBytes = buffer_size;

	cbv = dev.buffer_heap->GetNextHandle();

	dev.device->CreateConstantBufferView(&desc, cbv);
}