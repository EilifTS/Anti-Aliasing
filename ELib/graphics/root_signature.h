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
		inline void InitConstants(int number_of_constants, int shader_register) { return InitConstants(number_of_constants, shader_register, ShaderVisibility::All); };
		void InitConstants(int number_of_constants, int shader_register, ShaderVisibility visibility);

		inline void InitConstantBuffer(int shader_register) { return InitConstantBuffer(shader_register, ShaderVisibility::All); };
		void InitConstantBuffer(int shader_register, ShaderVisibility visibility);

		void InitShaderResource(int shader_register);

		inline void InitDescriptorTable(int shader_register) { return InitDescriptorTable(shader_register, ShaderVisibility::All); };
		inline void InitDescriptorTable(int shader_register, ShaderVisibility visibility) { return InitDescriptorTable(shader_register, 1, visibility); };
		void InitDescriptorTable(int shader_register, int num_entries, ShaderVisibility visibility);
		void InitConstantBufferTable(int shader_register, int num_entries, ShaderVisibility visibility);
		void InitUnorderedAccessTable(int shader_register, int num_entries, ShaderVisibility visibility);

		void AddSampler(const Sampler& sampler, int shader_register);

		inline void Finalize(Device& dev) { Finalize(dev, false); };
		void Finalize(Device& dev, bool is_local);

	private:
		ComPtr<ID3D12RootSignature> root_signature;
		std::vector<D3D12_ROOT_PARAMETER1> root_parameters;
		std::vector<std::shared_ptr<D3D12_DESCRIPTOR_RANGE1>> ranges;
		std::vector<D3D12_STATIC_SAMPLER_DESC> samplers;

		friend CommandContext;
		friend PipelineState;
		friend RTPipelineState;
	};
}