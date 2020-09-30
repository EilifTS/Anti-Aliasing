#include "shader_table.h"
#include "rt_pipeline_state.h"
#include "../texture2d.h"
#include "../unordered_access_buffer.h"
#include "../internal/egx_internal.h"
#include "../internal/upload_heap.h"
#include "tlas.h"

namespace
{
	int getShaderRecordSize(const std::vector<egx::ShaderTable::Entry>& entries)
	{
		int max_size = 0;
		for (const auto& e : entries)
			max_size = max(max_size, e.GetByteSize());

		max_size = egx::Align(max_size, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		return max_size;
	}
}

egx::ShaderTable::Entry::Entry(const std::wstring& name)
	: name(name), root_arguments(), byte_size((int)D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)
{

}

void egx::ShaderTable::Entry::AddDescriptorTable(Texture2D& start_texture)
{
	byte_size += 8;
	root_arguments.push_back((void*)start_texture.getSRVGPU().ptr);
}


void egx::ShaderTable::Entry::AddUnorderedAccessTable(UnorderedAccessBuffer& start_buffer)
{
	byte_size += 8;
	root_arguments.push_back((void*)start_buffer.uav_gpu.ptr);
}
void egx::ShaderTable::Entry::AddAccelerationStructure(TLAS& tlas)
{
	byte_size += 8;
	root_arguments.push_back((void*)tlas.srv_gpu.ptr);
}

int egx::ShaderTable::Entry::GetByteSize() const
{
	return byte_size;
}

egx::ShaderTable::Entry& egx::ShaderTable::AddRayGenerationProgram(const std::wstring& entry_point)
{
	ray_gen_entries.emplace_back(Entry(entry_point));
	return ray_gen_entries.back();
}
egx::ShaderTable::Entry& egx::ShaderTable::AddMissProgram(const std::wstring& entry_point)
{
	miss_entries.emplace_back(Entry(entry_point));
	return miss_entries.back();
}
egx::ShaderTable::Entry& egx::ShaderTable::AddHitProgram(const std::wstring& entry_point)
{
	hit_entries.emplace_back(Entry(entry_point));
	return hit_entries.back();
}

void egx::ShaderTable::Finalize(Device& dev, RTPipelineState& pipeline_state)
{
	ray_gen_entry_size = getShaderRecordSize(ray_gen_entries);
	miss_entry_size = getShaderRecordSize(miss_entries);
	hit_entry_size = getShaderRecordSize(hit_entries);

	ray_gen_table_size = ray_gen_entry_size * (int)ray_gen_entries.size();
	miss_table_size = miss_entry_size * (int)miss_entries.size();
	hit_table_size = hit_entry_size * (int)hit_entries.size();

	int table_size = ray_gen_table_size + miss_table_size + hit_table_size;
	table_size = Align(table_size, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

	shader_table_heap = std::make_shared<UploadHeap>(dev, table_size);

	uint8_t* ptr = (uint8_t*)shader_table_heap->Map();

	for (const auto& e : ray_gen_entries)
	{
		void* id = pipeline_state.state_object_props->GetShaderIdentifier(e.name.c_str());
		memcpy(ptr, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		memcpy(ptr + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, e.root_arguments.data(), e.root_arguments.size() * 8);
		ptr += ray_gen_entry_size;
	}

	for (const auto& e : miss_entries)
	{
		void* id = pipeline_state.state_object_props->GetShaderIdentifier(e.name.c_str());
		memcpy(ptr, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		memcpy(ptr + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, e.root_arguments.data(), e.root_arguments.size() * 8);
		ptr += miss_entry_size;
	}

	for (const auto& e : hit_entries)
	{
		void* id = pipeline_state.state_object_props->GetShaderIdentifier(e.name.c_str());
		memcpy(ptr, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		memcpy(ptr + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, e.root_arguments.data(), e.root_arguments.size() * 8);
		ptr += hit_entry_size;
	}

	shader_table_heap->Unmap();
}