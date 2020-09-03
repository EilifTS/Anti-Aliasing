#include "descriptor_heap.h"
#include "egx_internal.h"
#include "d3dx12.h"
#include <assert.h>

namespace
{
	inline D3D12_DESCRIPTOR_HEAP_TYPE convertType(egx::DescriptorType type)
	{
		switch (type)
		{
		case egx::DescriptorType::Buffer: return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		case egx::DescriptorType::Sampler: return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
		case egx::DescriptorType::RenderTarget: return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		case egx::DescriptorType::DepthStencil: return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		default: throw std::runtime_error("Invalid descriptor type");
		}
	}
}

egx::DescriptorHeap::DescriptorHeap(ComPtr<ID3D12Device8> device, DescriptorType type, int max_descriptors)
	: descriptor_size(0), max_descriptors(max_descriptors), num_descriptors(0)
{
	auto dx_type = convertType(type);
	descriptor_size = device->GetDescriptorHandleIncrementSize(dx_type);
	D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (type == DescriptorType::Buffer || type == DescriptorType::Sampler)
		flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = max_descriptors;
	desc.Type = dx_type;
	desc.Flags = flags;

	THROWIFFAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap)), "Failed to create descriptor heap");

	eio::Console::Log("Created: Descriptor heap");
}

D3D12_CPU_DESCRIPTOR_HANDLE egx::DescriptorHeap::GetNextHandle()
{
	assert(num_descriptors != max_descriptors);
	CD3DX12_CPU_DESCRIPTOR_HANDLE desc_h(descriptor_heap->GetCPUDescriptorHandleForHeapStart());

	desc_h.Offset(num_descriptors, descriptor_size);
	num_descriptors++;
	return desc_h;
}