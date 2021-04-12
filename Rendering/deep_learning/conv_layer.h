#pragma once
#include <DirectML.h>
#include <vector>
#include "dml_common.h"
#include "graphics/unordered_access_buffer.h"
#include "graphics/internal/descriptor_heap.h"

namespace egx
{
	class ConvLayer
	{
	public:
		ConvLayer(Device& dev, IDMLDevice* dml_dev, const DMLDims& input_dims, UINT output_channels, UINT filter_size, bool fuse_activation, UINT stride = 1);

		void UploadWeights(Device& dev, CommandContext& context, const std::vector<uint16_t>& weights);
		void UploadBias(Device& dev, CommandContext& context, const std::vector<uint16_t>& bias);

		void CreateBindingTable(IDMLDevice* dml_dev, DescriptorHeap& desc_heap, UINT index);
		void BindResources(ID3D12Resource* input, ID3D12Resource* output);

		const DMLDims& GetInputDims()const { return input_dims; };
		const DMLDims& GetOutputDims()const { return output_dims; };
		const DMLFilterDims& GetFilterDims()const { return filter_dims; };
		UINT64 GetInputBufferSize()const { return input_buffer_size; };
		UINT64 GetOutputBufferSize()const { return output_buffer_size; };

		IDMLCompiledOperator* GetCompiledOperator() {return compiled_op.Get(); };
		UnorderedAccessBuffer& GetWeightTensor() { return *weight_tensor_buffer.get(); };
		UnorderedAccessBuffer& GetBiasTensor() { return *bias_tensor_buffer.get(); };
		UnorderedAccessBuffer* GetPersistantResource() { return persistent_resource.get(); };
		UnorderedAccessBuffer* GetTemporaryResource() { return temporary_resource.get(); };
		UINT64 GetPersistantResourceSize() { return persistant_resource_size; };
		UINT64 GetTemporaryResourceSize() { return temporary_resource_size; };

		IDMLBindingTable* GetBindingTable() { return binding_table.Get(); };
	public:
		// Shared variables for initialization
		static ComPtr<IDMLOperatorInitializer> conv_initializer;
		static ComPtr<IDMLBindingTable> temp_binding_table;
		static UINT64 initializer_temp_res_size;
		static UINT descriptor_start;
		static UINT descriptor_count;
		static std::unique_ptr<UnorderedAccessBuffer> initializer_temp_res;
		static void CreateConvInitializer(Device& dev, IDMLDevice* dml_dev, std::vector<ConvLayer>& layers);
		static int GetDescriptorCount(std::vector<ConvLayer>& layers);
		static void InitializeConvLayers(IDMLDevice* dml_dev, IDMLCommandRecorder* command_recorder, ID3D12CommandList* command_list, DescriptorHeap& desc_heap, std::vector<ConvLayer>& layers);

	private:
		DMLDims input_dims;
		DMLDims output_dims;
		DMLFilterDims filter_dims;
		UINT64 input_buffer_size;
		UINT64 output_buffer_size;

		ComPtr<IDMLCompiledOperator> compiled_op;
		std::unique_ptr<UnorderedAccessBuffer> bias_tensor_buffer;
		std::unique_ptr<UnorderedAccessBuffer> weight_tensor_buffer;

		UINT64 persistant_resource_size;
		UINT64 temporary_resource_size;
		std::unique_ptr<UnorderedAccessBuffer> persistent_resource;
		std::unique_ptr<UnorderedAccessBuffer> temporary_resource;

		ComPtr<IDMLBindingTable> binding_table;
	};
}