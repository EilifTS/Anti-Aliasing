#pragma once
#include "internal/gpu_buffer.h"

namespace egx
{

	class IndexBuffer : public GPUBuffer
	{
	public:
		IndexBuffer(Device& dev, int index_count)
			: GPUBuffer(
				dev,
				D3D12_RESOURCE_DIMENSION_BUFFER,
				DXGI_FORMAT_UNKNOWN,
				index_count, 1, 1, (int)sizeof(unsigned int),
				D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				D3D12_RESOURCE_FLAG_NONE,
				nullptr),
			view()
		{
			view.Format = DXGI_FORMAT_R32_UINT;
			view.BufferLocation = buffer->GetGPUVirtualAddress();
			view.SizeInBytes = index_count * sizeof(unsigned int);
		}

	private:
		D3D12_INDEX_BUFFER_VIEW view;

		friend CommandContext;
	};
}