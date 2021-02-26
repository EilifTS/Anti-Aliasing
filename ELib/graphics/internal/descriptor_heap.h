#pragma once
#include "egx_common.h"

namespace egx
{
	enum class DescriptorType
	{
		Buffer, Sampler, RenderTarget, DepthStencil
	};

	class DescriptorHeap
	{
	public:
		DescriptorHeap(ComPtr<ID3D12Device5> device, DescriptorType type, int max_descriptors);
		D3D12_CPU_DESCRIPTOR_HANDLE GetNextHandle();
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandleByIndex(UINT index);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandleByIndex(UINT index);

	private:
		ComPtr<ID3D12DescriptorHeap> descriptor_heap;
		unsigned int descriptor_size;
		int max_descriptors;
		int num_descriptors;

		friend MasterNet;
		friend CommandContext;
	};
}