#pragma once
#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace egx
{
	class Device;
	class GPUBuffer
	{
	public:
		GPUBuffer(
			Device& dev, 
			D3D12_RESOURCE_DIMENSION dimension, 
			DXGI_FORMAT format, 
			int width, int height, int depth,
			int element_size,
			D3D12_TEXTURE_LAYOUT layout, 
			D3D12_RESOURCE_FLAGS flags);

		int GetElementSize() const { return element_size; };
		int GetElementCount() const { return element_count; };
		int GetBufferSize() const { return element_size * element_count; };

	protected:
		ComPtr<ID3D12Resource> buffer;
		int element_size;
		int element_count;

	private:
		friend Device;
	};
}