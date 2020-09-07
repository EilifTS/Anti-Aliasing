#pragma once
#include "egx_common.h"

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

	private:
		friend CommandContext;
	};
}