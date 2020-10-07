#include "upload_heap.h"
#include "egx_internal.h"
#include "../device.h"

egx::UploadHeap::UploadHeap(Device& dev, int buffer_size)
	: buffer_size(buffer_size)
{
	D3D12_RESOURCE_DESC desc = {};

	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = buffer_size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	THROWIFFAILED(dev.device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer)),
		"Failed to create upload heap");

	//eio::Console::Log("Created: Upload heap");
}

void* egx::UploadHeap::Map()
{
	void* ptr;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	THROWIFFAILED(buffer->Map(0, &readRange, &ptr), "Failed to map resource");
	return ptr;
}

void egx::UploadHeap::Unmap()
{
	buffer->Unmap(0, nullptr);
}

egx::DynamicUploadHeap::DynamicUploadHeap(Device& dev, int heap_size)
	: heap_size(heap_size), remaining_space(heap_size), current_res_offset(0)
{
	heaps.emplace_back(dev, heap_size);
	current_ptr = heaps.back().Map();
}

void* egx::DynamicUploadHeap::ReserveSpace(Device& dev, int space)
{
	space = Align(space, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
	assert(space <= heap_size);
	if (remaining_space < space)
	{
		heaps.back().Unmap();
		heaps.emplace_back(dev, heap_size);
		remaining_space = heap_size;
		current_ptr = heaps.back().Map();
	}
	void* reserved_ptr = current_ptr;
	current_res_offset = heap_size - remaining_space;
	remaining_space -= space;
	current_ptr = ((char*)current_ptr + space);
	return reserved_ptr;
}

void egx::DynamicUploadHeap::Clear()
{
	if (heaps.size() == 1)
	{
		heaps.back().Unmap();
	}
	else
	{
		heaps.erase(std::begin(heaps) + 1, std::end(heaps));
	}
	remaining_space = heap_size;
	current_ptr = heaps.back().Map();
	current_res_offset = 0;
}