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

			//void AddConstant();
			void AddConstantBuffer(ConstantBuffer& buffer);
			//void AddRootSRV(Texture2D& start_texture);
			void AddDescriptorTable(Texture2D& start_texture);
			void AddUnorderedAccessTable(UnorderedAccessBuffer& start_buffer);
			void AddAccelerationStructure(TLAS& tlas);
			void AddVertexBuffer(VertexBuffer& buffer);
			void AddIndexBuffer(IndexBuffer& buffer);

			int GetByteSize() const;

		private:
			std::wstring name;
			std::vector<void*> root_arguments;
			int byte_size;
			
			friend ShaderTable;
		};

	public:
		Entry& AddRayGenerationProgram(const std::wstring& entry_point);
		Entry& AddMissProgram(const std::wstring& entry_point);
		Entry& AddHitProgram(const std::wstring& entry_point);
		void Finalize(Device& dev, RTPipelineState& pipeline_state);

	private:
		std::vector<Entry> ray_gen_entries;
		std::vector<Entry> miss_entries;
		std::vector<Entry> hit_entries;
		std::shared_ptr<UploadHeap> shader_table_heap;

		int ray_gen_table_start = 0;
		int ray_gen_table_size = 0;
		int ray_gen_entry_size = 0;
		int miss_table_start = 0;
		int miss_table_size = 0;
		int miss_entry_size = 0;
		int hit_table_start = 0;
		int hit_table_size = 0;
		int hit_entry_size = 0;

	private:
		friend CommandContext;
	};
}