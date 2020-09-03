#pragma once
#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace egx
{
	class Device;
	class UploadHeap
	{
	public:
		UploadHeap(Device& dev, int buffer_size);

		void* Map();
		void Unmap();

		int GetBufferSize() const { return buffer_size; };

	protected:
		ComPtr<ID3D12Resource> buffer;
		int buffer_size;

	};
}