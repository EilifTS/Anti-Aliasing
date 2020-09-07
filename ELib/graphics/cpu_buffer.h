#pragma once

namespace egx
{
	class CPUBuffer
	{
	public:
		CPUBuffer(void* ptr, int buffer_size)
			: ptr(ptr), buffer_size(buffer_size)
		{}

		const void* GetPtr() const { return ptr; };
		int Size() const { return buffer_size; };

	private:
		void* ptr;
		int buffer_size;
	};
}