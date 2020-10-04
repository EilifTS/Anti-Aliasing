#pragma once
#include "internal/gpu_buffer.h"

namespace egx
{

	class ConstantBuffer : public GPUBuffer
	{
	public:
		ConstantBuffer(Device& dev, int buffer_size);
		
	private:
		D3D12_CPU_DESCRIPTOR_HANDLE cbv;
		friend CommandContext;
		friend ShaderTable;
	};
}