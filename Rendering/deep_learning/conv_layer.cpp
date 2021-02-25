#include "conv_layer.h"
#include "graphics/internal/egx_internal.h"

namespace
{
	const egx::Layout tensor_layout = egx::Layout::NHWC;
	const DML_TENSOR_DATA_TYPE tensor_type = DML_TENSOR_DATA_TYPE_FLOAT16;
}

egx::ConvLayer::ConvLayer(Device& dev, IDMLDevice* dml_dev, const DMLDims& input_dims, UINT output_channels)
{
	filter_dims = { output_channels, input_dims.dims[1], 3, 3 };
	this->input_dims = { input_dims.dims[0], input_dims.dims[1], input_dims.dims[2], input_dims.dims[3] };
	output_dims = { input_dims.dims[0], output_channels, input_dims.dims[2], input_dims.dims[3] };

	// Describe input and output tensors
	auto input_strides = CalculateStrides(
		tensor_layout,
		{ input_dims.dims[0], input_dims.dims[1], input_dims.dims[2], input_dims.dims[3] },
		{ false, false, false, false });

	input_buffer_size = DMLCalcBufferTensorSize(tensor_type, 4, input_dims.dims, input_strides.data());

	DML_BUFFER_TENSOR_DESC input_buffer_desc = {};
	input_buffer_desc.DataType = tensor_type;
	input_buffer_desc.Flags = DML_TENSOR_FLAG_NONE;
	input_buffer_desc.DimensionCount = 4;
	input_buffer_desc.Sizes = input_dims.dims;
	input_buffer_desc.Strides = input_strides.data();
	input_buffer_desc.TotalTensorSizeInBytes = input_buffer_size;
	DML_TENSOR_DESC input_desc = {};
	input_desc.Type = DML_TENSOR_TYPE_BUFFER;
	input_desc.Desc = &input_buffer_desc;


	auto output_strides = CalculateStrides(
		tensor_layout,
		{ output_dims.dims[0], output_dims.dims[1], output_dims.dims[2], output_dims.dims[3] },
		{ false, false, false, false });

	output_buffer_size = DMLCalcBufferTensorSize(tensor_type, 4, output_dims.dims, output_strides.data());

	DML_BUFFER_TENSOR_DESC output_buffer_desc = {};
	output_buffer_desc.DataType = tensor_type;
	output_buffer_desc.Flags = DML_TENSOR_FLAG_NONE;
	output_buffer_desc.DimensionCount = 4;
	output_buffer_desc.Sizes = output_dims.dims;
	output_buffer_desc.Strides = output_strides.data();
	output_buffer_desc.TotalTensorSizeInBytes = output_buffer_size;
	DML_TENSOR_DESC output_desc = {};
	output_desc.Type = DML_TENSOR_TYPE_BUFFER;
	output_desc.Desc = &output_buffer_desc;

	// Describe weight tensor
	auto filter_strides = CalculateStrides(
		tensor_layout,
		{ filter_dims.dims[0], filter_dims.dims[1], filter_dims.dims[2], filter_dims.dims[3] },
		{ false, false, false, false });

	UINT64 filter_buffer_size = DMLCalcBufferTensorSize(tensor_type, 4, filter_dims.dims, filter_strides.data());

	DML_BUFFER_TENSOR_DESC filter_buffer_desc = {};
	filter_buffer_desc.DataType = tensor_type;
	filter_buffer_desc.Flags = DML_TENSOR_FLAG_OWNED_BY_DML;
	filter_buffer_desc.DimensionCount = 4;
	filter_buffer_desc.Sizes = filter_dims.dims;
	filter_buffer_desc.Strides = filter_strides.data();
	filter_buffer_desc.TotalTensorSizeInBytes = filter_buffer_size;
	DML_TENSOR_DESC filter_desc = {};
	filter_desc.Type = DML_TENSOR_TYPE_BUFFER;
	filter_desc.Desc = &filter_buffer_desc;

	// Describe bias
	UINT bias_dims[] = { 1, filter_dims.dims[0], 1, 1 };
	auto bias_strides = CalculateStrides(
		tensor_layout,
		{ bias_dims[0], bias_dims[1], bias_dims[2], bias_dims[3] },
		{ false, false, false, false });
	UINT64 bias_buffer_size = DMLCalcBufferTensorSize(tensor_type, 4, bias_dims, bias_strides.data());

	DML_BUFFER_TENSOR_DESC bias_buffer_desc = {};
	bias_buffer_desc.DataType = tensor_type;
	bias_buffer_desc.Flags = DML_TENSOR_FLAG_OWNED_BY_DML;
	bias_buffer_desc.DimensionCount = 4;
	bias_buffer_desc.Sizes = bias_dims;
	bias_buffer_desc.Strides = bias_strides.data();
	bias_buffer_desc.TotalTensorSizeInBytes = bias_buffer_size;
	DML_TENSOR_DESC bias_desc = {};
	bias_desc.Type = DML_TENSOR_TYPE_BUFFER;
	bias_desc.Desc = &bias_buffer_desc;

	// Describe, create and compile conv operator
	UINT strides[] = { 1, 1 };
	UINT dilations[] = { 1, 1 };
	UINT start_padding[] = { 1, 1 };
	UINT end_padding[] = { 1, 1 };
	UINT output_padding[] = { 0, 0 };

	DML_ACTIVATION_RELU_OPERATOR_DESC fused_relu_desc = { 0 };
	DML_OPERATOR_DESC activation_desc = { DML_OPERATOR_ACTIVATION_RELU, &fused_relu_desc };

	DML_CONVOLUTION_OPERATOR_DESC conv_desc = {};
	conv_desc.InputTensor = &input_desc;
	conv_desc.FilterTensor = &filter_desc;
	conv_desc.BiasTensor = &bias_desc;
	conv_desc.OutputTensor = &output_desc;
	conv_desc.Mode = DML_CONVOLUTION_MODE_CROSS_CORRELATION;
	conv_desc.Direction = DML_CONVOLUTION_DIRECTION_FORWARD;
	conv_desc.DimensionCount = 2;
	conv_desc.Strides = strides;
	conv_desc.Dilations = dilations;
	conv_desc.StartPadding = start_padding;
	conv_desc.EndPadding = end_padding;
	conv_desc.OutputPadding = output_padding;
	conv_desc.GroupCount = 1;
	conv_desc.FusedActivation = &activation_desc;

	DML_OPERATOR_DESC op_desc = { DML_OPERATOR_CONVOLUTION, &conv_desc };

	ComPtr<IDMLOperator> op;
	THROWIFFAILED(dml_dev->CreateOperator(&op_desc, IID_PPV_ARGS(&op)), "Failed to create DML convolution operator");
	THROWIFFAILED(dml_dev->CompileOperator(op.Get(), DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION, IID_PPV_ARGS(&compiled_op))
		, "Failed to compile DML convolution operator");

	weight_tensor_buffer = std::make_unique<UnorderedAccessBuffer>(dev, filter_buffer_size);
	bias_tensor_buffer = std::make_unique<UnorderedAccessBuffer>(dev, bias_buffer_size);
}