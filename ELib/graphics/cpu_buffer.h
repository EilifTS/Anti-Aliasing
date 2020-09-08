#pragma once

namespace egx
{
	class CPUBuffer
	{
	public:
		CPUBuffer(const void* ptr, int buffer_size)
			: ptr(ptr), buffer_size(buffer_size)
		{}

		const void* GetPtr() const { return ptr; };
		int Size() const { return buffer_size; };

	private:
		const void* ptr;
		int buffer_size;
	};
}