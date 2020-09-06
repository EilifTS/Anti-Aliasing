#pragma once
#include "internal/gpu_buffer.h"

namespace egx
{

	class VertexBuffer : public GPUBuffer
	{
	public:
		VertexBuffer(Device& dev, int vertex_size, int vertex_count) 
			: GPUBuffer(
			dev,
			D3D12_RESOURCE_DIMENSION_BUFFER,
			DXGI_FORMAT_UNKNOWN,
			vertex_count * vertex_size, 1, 1, 1,
			D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			D3D12_RESOURCE_FLAG_NONE),
			view()
		{
			view.BufferLocation = buffer->GetGPUVirtualAddress();
			view.StrideInBytes = vertex_size;
			view.SizeInBytes = vertex_size * vertex_count;
		}

	private:
		D3D12_VERTEX_BUFFER_VIEW view;

		friend CommandContext;
	};
}