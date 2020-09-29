#pragma once
#include <string>
#include <vector>
#include "../internal/egx_common.h"

namespace egx
{
	class ShaderTable
	{
	public:
		class Entry
		{
		public:
			Entry(const std::wstring& name);

			void AddConstant();
			void AddConstantBuffer();
			void AddDescriptorTable();

			int GetByteSize() const;

		private:
			std::wstring name;
			std::vector<void*> root_arguments;
			int byte_size;
		};

	public:
		Entry& AddShader(const std::wstring& entry_point);
		void Finalize(Device& dev);

	private:
		std::vector<Entry> entries;
		std::shared_ptr<UploadHeap> shader_table_heap;
	};
}