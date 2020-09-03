#pragma once
#include "internal/gpu_buffer.h"

namespace egx
{

	class ConstantBuffer : public GPUBuffer
	{
	public:
		ConstantBuffer(Device& dev, int buffer_size)
			: GPUBuffer(
				dev, 
				D3D12_RESOURCE_DIMENSION_BUFFER,
				DXGI_FORMAT_UNKNOWN,
				buffer_size, 1, 1, 1,
				D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				D3D12_RESOURCE_FLAG_NONE)
		{

		}

	};
}