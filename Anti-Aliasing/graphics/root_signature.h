#pragma once
#include <vector>
#include "internal/egx_common.h"
#include "sampler.h"

namespace egx
{
	class RootSignature
	{
	public:
		RootSignature();
		void InitConstants(int number_of_constants, int shader_register);
		void InitConstantBuffer(int shader_register);
		void InitShaderResource(int shader_register);
		// TODO: Create this one if needed
		//void InitDescriptorTable();

		void AddSampler(const Sampler& sampler, int shader_register);

		void Finalize(Device& dev);

	private:
		ComPtr<ID3D12RootSignature> root_signature;
		std::vector<D3D12_ROOT_PARAMETER1> root_parameters;
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;
	};
}