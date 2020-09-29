#include "shader_table.h"

egx::ShaderTable::Entry::Entry(const std::wstring& name)
	: name(name), root_arguments(), byte_size((int)D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES)
{

}

void egx::ShaderTable::Entry::AddConstant()
{

}
void egx::ShaderTable::Entry::AddConstantBuffer()
{

}
void egx::ShaderTable::Entry::AddDescriptorTable()
{

}

int egx::ShaderTable::Entry::GetByteSize() const
{

}