#include "gpu_buffer.h"
#include "egx_internal.h"
#include "../device.h"

egx::GPUBuffer::GPUBuffer(
	Device& dev, 
	D3D12_RESOURCE_DIMENSION dimension, 
	DXGI_FORMAT format, 
	int width, int height, int depth, 
	int element_size, 
	D3D12_TEXTURE_LAYOUT layout, 
	D3D12_RESOURCE_FLAGS flags)
	: element_count(width * height * depth), element_size(element_size),
	state(D3D12_RESOURCE_STATE_COPY_DEST)
{
	D3D12_RESOURCE_DESC desc = {};

	desc.Dimension = dimension;
	desc.Alignment = 0;
	desc.Width = width * element_size;
	desc.Height = height;
	desc.DepthOrArraySize = depth;
	desc.MipLevels = 1;
	desc.Format = format;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = layout;
	desc.Flags = flags;

	THROWIFFAILED(dev.device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		state,
		nullptr,
		IID_PPV_ARGS(&buffer)),
		"Failed to create GPU buffer");

	eio::Console::Log("Created: GPU buffer");
}

egx::GPUBuffer::GPUBuffer(ComPtr<ID3D12Resource> buffer)
	: buffer(buffer), element_count(0), element_size(0),
	state(D3D12_RESOURCE_STATE_PRESENT)
{
	
}