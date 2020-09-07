#pragma once
#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace egx
{
	enum class DescriptorType
	{
		Buffer, Sampler, RenderTarget, DepthStencil
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap(ComPtr<ID3D12Device6> device, DescriptorType type, int max_descriptors);
		D3D12_CPU_DESCRIPTOR_HANDLE GetNextHandle();

	private:
		ComPtr<ID3D12DescriptorHeap> descriptor_heap;
		unsigned int descriptor_size;
		int max_descriptors;
		int num_descriptors;
	};
}