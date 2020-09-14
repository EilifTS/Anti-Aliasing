#include "gpu_buffer.h"
#include "egx_internal.h"
#include "../device.h"

namespace
{
	D3D12_CLEAR_VALUE getClearValue(DXGI_FORMAT format, egx::ClearValue clear_val)
	{
		float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		float blue[4] = { 0.117f, 0.565f, 1.0f, 1.0f };
		switch (clear_val)
		{
		case egx::ClearValue::Depth0: return CD3DX12_CLEAR_VALUE(format, 0.0f, 0);
		case egx::ClearValue::Depth1: return CD3DX12_CLEAR_VALUE(format, 1.0f, 0);
		case egx::ClearValue::ColorBlack: return CD3DX12_CLEAR_VALUE(format, black);
		case egx::ClearValue::ColorBlue: return CD3DX12_CLEAR_VALUE(format, blue);
		case egx::ClearValue::None: return CD3DX12_CLEAR_VALUE();
		default:
			throw std::runtime_error("Invalid clear value");
		}
	}
}

egx::GPUBuffer::GPUBuffer(
	Device& dev, 
	D3D12_RESOURCE_DIMENSION dimension, 
	DXGI_FORMAT format, 
	int width, int height, int depth, 
	int element_size, 
	D3D12_TEXTURE_LAYOUT layout, 
	D3D12_RESOURCE_FLAGS flags,
	ClearValue clear_value)
	: element_count(width * height * depth), element_size(element_size),
	state(D3D12_RESOURCE_STATE_COPY_DEST)
{
	D3D12_RESOURCE_DESC desc = {};

	desc.Dimension = dimension;
	desc.Alignment = 0;
	desc.Width = (UINT64)width * element_size;
	desc.Height = height;
	desc.DepthOrArraySize = depth;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = layout;
	desc.Flags = flags;

	// Get the right clear value
	D3D12_CLEAR_VALUE xclear_value = getClearValue(format, clear_value);
	D3D12_CLEAR_VALUE* pclear_value = &xclear_value;
	if (clear_value == ClearValue::None) pclear_value = nullptr;

	THROWIFFAILED(dev.device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		state,
		pclear_value,
		IID_PPV_ARGS(&buffer)),
		"Failed to create GPU buffer");

	eio::Console::Log("Created: GPU buffer");
}

egx::GPUBuffer::GPUBuffer(ComPtr<ID3D12Resource> buffer, D3D12_RESOURCE_STATES start_state)
	: buffer(buffer), element_count(0), element_size(0),
	state(start_state)
{
	auto desc = buffer->GetDesc();
	element_count = (int)desc.Width * desc.Height * desc.DepthOrArraySize;
	element_size = formatByteSize(desc.Format);
}