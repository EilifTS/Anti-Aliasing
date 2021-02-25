#pragma once
#include <DirectML.h>
#include "dml_common.h"
#include "graphics/unordered_access_buffer.h"

namespace egx
{
	class ConvLayer
	{
	public:
		ConvLayer(Device& dev, IDMLDevice* dml_dev, const DMLDims& input_dims, UINT output_channels);

		const DMLDims& GetInputDims()const { return input_dims; };
		const DMLDims& GetOutputDims()const { return output_dims; };
		const DMLFilterDims& GetFilterDims()const { return filter_dims; };
		UINT64 GetInputBufferSize()const { return input_buffer_size; };
		UINT64 GetOutputBufferSize()const { return output_buffer_size; };

		IDMLCompiledOperator* GetCompiledOperator() {return compiled_op.Get(); };
		UnorderedAccessBuffer& GetWeightTensor() { return *weight_tensor_buffer.get(); };
		UnorderedAccessBuffer& GetBiasTensor() { return *bias_tensor_buffer.get(); };
	private:
		DMLDims input_dims;
		DMLDims output_dims;
		DMLFilterDims filter_dims;
		UINT64 input_buffer_size;
		UINT64 output_buffer_size;

		ComPtr<IDMLCompiledOperator> compiled_op;
		std::unique_ptr<UnorderedAccessBuffer> bias_tensor_buffer;
		std::unique_ptr<UnorderedAccessBuffer> weight_tensor_buffer;
	};
}