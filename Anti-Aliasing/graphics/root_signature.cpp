#include "root_signature.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::RootSignature::RootSignature()
	: root_signature(nullptr), root_parameters(), samplers()
{

}
void egx::RootSignature::InitConstants(int number_of_constants, int shader_register)
{
	CD3DX12_ROOT_PARAMETER1 param;
	param.InitAsConstants(number_of_constants, shader_register, 0, D3D12_SHADER_VISIBILITY_ALL);
	root_parameters.push_back(param);
}
void egx::RootSignature::InitConstantBuffer(int shader_register)
{
	CD3DX12_ROOT_PARAMETER1 param;
	param.InitAsConstantBufferView(shader_register, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	root_parameters.push_back(param);
}
void egx::RootSignature::InitShaderResource(int shader_register)
{
	CD3DX12_ROOT_PARAMETER1 param;
	param.InitAsShaderResourceView(shader_register, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_ALL);
	root_parameters.push_back(param);
}
void egx::RootSignature::AddSampler(const Sampler& sampler, int shader_register)
{
	auto sampler_desc = sampler.desc;
	sampler_desc.ShaderRegister = shader_register;
	sampler_desc.RegisterSpace = 0;
	samplers.push_back(sampler_desc);
}

void egx::RootSignature::Finalize(Device& dev)
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
	feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(dev.device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data))))
	{
		feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}
	
	// Only allow vertex and pixel shaders for now
	D3D12_ROOT_SIGNATURE_FLAGS root_flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;


	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_desc;
	root_desc.Init_1_1((UINT)root_parameters.size(), root_parameters.data(), (UINT)samplers.size(), samplers.data(), root_flags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> signature_blob;
	ComPtr<ID3DBlob> error_blob;
	THROWIFFAILED(D3DX12SerializeVersionedRootSignature(&root_desc,
		feature_data.HighestVersion, &signature_blob, &error_blob), "Failed to serialize root signature");

	// Create the root signature.
	THROWIFFAILED(dev.device->CreateRootSignature(0, signature_blob->GetBufferPointer(),
		signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature)), "Failed to create root signature");
}