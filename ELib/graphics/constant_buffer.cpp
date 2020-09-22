#include "constant_buffer.h"
#include "internal/egx_internal.h"
#include "device.h"

namespace
{
	int alignBufferSize(int buffer_size)
	{
		if (buffer_size == 0)
			throw std::runtime_error("Invalid buffer size");

		int rem = buffer_size & 0xff;
		if (rem == 0)
			return buffer_size;

		buffer_size += (rem ^ 0xff) + 1;
		return buffer_size;
	}
}

egx::ConstantBuffer::ConstantBuffer(Device& dev, int buffer_size)
	: GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_BUFFER,
		DXGI_FORMAT_UNKNOWN,
		alignBufferSize(buffer_size), 1, 1, 1,
		D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		D3D12_RESOURCE_FLAG_NONE,
		nullptr), cbv()
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
	desc.BufferLocation = buffer->GetGPUVirtualAddress();
	desc.SizeInBytes = alignBufferSize(buffer_size);

	cbv = dev.buffer_heap->GetNextHandle();

	dev.device->CreateConstantBufferView(&desc, cbv);
}