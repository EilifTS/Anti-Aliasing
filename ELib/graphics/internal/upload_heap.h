#pragma once
#include "egx_common.h"
#include <vector>

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
		friend TLAS;
	};

	class DynamicUploadHeap
	{
	public:
		DynamicUploadHeap(Device& dev, int heap_size);

		void* ReserveSpace(Device& dev, int space);
		int GetCurrentReservationOffset() const { return current_res_offset; };
		UploadHeap& GetCurrentHeap() { return heaps.back(); };


		void Clear();

	private:
		int heap_size;
		int remaining_space;
		int current_res_offset;
		void* current_ptr;
		
		std::vector<UploadHeap> heaps;
	};
}