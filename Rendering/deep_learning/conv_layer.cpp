#define NOMINMAX
#include "conv_layer.h"
#include "graphics/internal/egx_internal.h"
#include "graphics/cpu_buffer.h"
#include "graphics/device.h"
#include "graphics/command_context.h"

ComPtr<IDMLOperatorInitializer> egx::ConvLayer::conv_initializer = nullptr;
ComPtr<IDMLBindingTable> egx::ConvLayer::temp_binding_table = nullptr;
UINT64 egx::ConvLayer::initializer_temp_res_size = 0;
UINT egx::ConvLayer::descriptor_start = 0;
UINT egx::ConvLayer::descriptor_count = 0;
std::unique_ptr<egx::UnorderedAccessBuffer> egx::ConvLayer::initializer_temp_res = nullptr;

namespace
{
	const egx::Layout tensor_layout = egx::Layout::NHWC;
	const DML_TENSOR_DATA_TYPE tensor_type = DML_TENSOR_DATA_TYPE_FLOAT16;
}

egx::ConvLayer::ConvLayer(Device& dev, IDMLDevice* dml_dev, const DMLDims& input_dims, UINT output_channels, UINT filter_size, bool fuse_activation)
{
	filter_dims = { output_channels, input_dims.dims[1], filter_size, filter_size };
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
	UINT padding_size = static_cast<UINT>(ceil((filter_size - 1) / 2.0f));
	UINT strides[] = { 1, 1 };
	UINT dilations[] = { 1, 1 };
	UINT start_padding[] = { padding_size, padding_size };
	UINT end_padding[] = { padding_size, padding_size };
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
	conv_desc.FusedActivation = fuse_activation ? &activation_desc : nullptr;

	DML_OPERATOR_DESC op_desc = { DML_OPERATOR_CONVOLUTION, &conv_desc };

	ComPtr<IDMLOperator> op;
	THROWIFFAILED(dml_dev->CreateOperator(&op_desc, IID_PPV_ARGS(&op)), "Failed to create DML convolution operator");
	THROWIFFAILED(dml_dev->CompileOperator(op.Get(), DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION, IID_PPV_ARGS(&compiled_op))
		, "Failed to compile DML convolution operator");

	weight_tensor_buffer = std::make_unique<UnorderedAccessBuffer>(dev, filter_buffer_size);
	bias_tensor_buffer = std::make_unique<UnorderedAccessBuffer>(dev, bias_buffer_size);

	// Create persistant resource
	auto binding_props = compiled_op->GetBindingProperties();
	persistant_resource_size = binding_props.PersistentResourceSize;
	if(persistant_resource_size > 0)
		persistent_resource = std::make_unique<UnorderedAccessBuffer>(dev, persistant_resource_size);

	temporary_resource_size = binding_props.TemporaryResourceSize;
	if (temporary_resource_size > 0)
		temporary_resource = std::make_unique<UnorderedAccessBuffer>(dev, temporary_resource_size);
}

void egx::ConvLayer::CreateBindingTable(IDMLDevice* dml_dev, DescriptorHeap& desc_heap, UINT index)
{
	DML_BINDING_TABLE_DESC binding_table_desc = {};
	binding_table_desc.Dispatchable = compiled_op.Get();
	binding_table_desc.CPUDescriptorHandle = desc_heap.GetCPUHandleByIndex(descriptor_start + descriptor_count * index);
	binding_table_desc.GPUDescriptorHandle = desc_heap.GetGPUHandleByIndex(descriptor_start + descriptor_count * index);
	binding_table_desc.SizeInDescriptors = descriptor_count;


	THROWIFFAILED(
		dml_dev->CreateBindingTable(
			&binding_table_desc,
			IID_PPV_ARGS(&binding_table)
		), "Failed to create DML binding table");
}

void egx::ConvLayer::BindResources(ID3D12Resource* input, ID3D12Resource* output)
{
	if (GetTemporaryResourceSize() != 0)
	{
		DML_BUFFER_BINDING buffer_binding{ GetTemporaryResource()->buffer.Get(), 0, GetTemporaryResourceSize() };
		DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
		GetBindingTable()->BindTemporaryResource(&binding_desc);
	}
	if (GetPersistantResourceSize() != 0)
	{
		DML_BUFFER_BINDING buffer_binding{ GetPersistantResource()->buffer.Get(), 0, GetPersistantResourceSize() };
		DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
		GetBindingTable()->BindPersistentResource(&binding_desc);
	}


	// Bind input
	DML_BINDING_DESC empty_binding_desc = { DML_BINDING_TYPE_NONE, nullptr };
	DML_BUFFER_BINDING input_buffer_binding{ input, 0, GetInputBufferSize() };
	DML_BINDING_DESC input_binding_desc[] = { { DML_BINDING_TYPE_BUFFER, &input_buffer_binding }, empty_binding_desc, empty_binding_desc };
	GetBindingTable()->BindInputs(3, input_binding_desc);

	// Bind output
	DML_BUFFER_BINDING output_buffer_binding{ output, 0, GetOutputBufferSize() };
	DML_BINDING_DESC output_binding_desc{ DML_BINDING_TYPE_BUFFER, &output_buffer_binding };
	GetBindingTable()->BindOutputs(1, &output_binding_desc);
}

void egx::ConvLayer::UploadWeights(Device& dev, CommandContext& context, const std::vector<uint16_t>& weights)
{
	// Change weights from NCHW to NHWC
	std::vector<uint16_t> transformed_weights(weights.size());
	UINT N = filter_dims.dims[0];
	UINT C = filter_dims.dims[1];
	UINT H = filter_dims.dims[2];
	UINT W = filter_dims.dims[3];
	for (UINT n = 0; n < N; n++)
	{
		for (UINT c = 0; c < C; c++)
		{
			for (UINT h = 0; h < H; h++)
			{
				for (UINT w = 0; w < W; w++)
				{
					UINT nchw_index =
						n * C * H * W +
						c * H * W +
						h * W + w;
					UINT nhwc_index = 
						n * C * H * W +
						h * C * W + 
						w * C + c;
					transformed_weights[nhwc_index] = weights[nchw_index];
				}
			}
		}
	}

	egx::CPUBuffer cpu_buffer(transformed_weights.data(), (int)sizeof(uint16_t)* transformed_weights.size());
	context.SetTransitionBuffer(*weight_tensor_buffer, egx::GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, *weight_tensor_buffer);
	context.SetTransitionBuffer(*weight_tensor_buffer, egx::GPUBufferState::UnorderedAccess);
}
void egx::ConvLayer::UploadBias(Device& dev, CommandContext& context, const std::vector<uint16_t>& bias)
{
	egx::CPUBuffer cpu_buffer(bias.data(), (int)sizeof(uint16_t) * bias.size());
	context.SetTransitionBuffer(*bias_tensor_buffer, egx::GPUBufferState::CopyDest);
	dev.ScheduleUpload(context, cpu_buffer, *bias_tensor_buffer);
	context.SetTransitionBuffer(*bias_tensor_buffer, egx::GPUBufferState::UnorderedAccess);
}

void egx::ConvLayer::CreateConvInitializer(Device& dev, IDMLDevice* dml_dev, std::vector<ConvLayer>& layers)
{
	std::vector<IDMLCompiledOperator*> comp_ops(layers.size());
	for (int i = 0; i < layers.size(); i++)
		comp_ops[i] = layers[i].GetCompiledOperator();

	THROWIFFAILED(
		dml_dev->CreateOperatorInitializer(
			comp_ops.size(),
			comp_ops.data(),
			IID_PPV_ARGS(&conv_initializer)
		), "Failed to create DML operator initializer");


	auto binding_props = conv_initializer->GetBindingProperties();
	initializer_temp_res_size = binding_props.TemporaryResourceSize;
	if (initializer_temp_res_size > 0)
		initializer_temp_res = std::make_unique<UnorderedAccessBuffer>(dev, initializer_temp_res_size);
}
int egx::ConvLayer::GetDescriptorCount(std::vector<ConvLayer>& layers)
{
	UINT max_desc_count = conv_initializer->GetBindingProperties().RequiredDescriptorCount;

	for (auto& layer : layers)
		max_desc_count = std::max(max_desc_count, layer.GetCompiledOperator()->GetBindingProperties().RequiredDescriptorCount);
	return max_desc_count;
}

void egx::ConvLayer::InitializeConvLayers(IDMLDevice* dml_dev, IDMLCommandRecorder* command_recorder, ID3D12CommandList* command_list, DescriptorHeap& desc_heap, std::vector<ConvLayer>& layers)
{
	// Create binding table
	ComPtr<IDMLBindingTable> temp_binding_table;
	DML_BINDING_TABLE_DESC binding_table_desc = {};
	binding_table_desc.Dispatchable = ConvLayer::conv_initializer.Get();
	binding_table_desc.CPUDescriptorHandle = desc_heap.GetCPUHandleByIndex(descriptor_start);
	binding_table_desc.GPUDescriptorHandle = desc_heap.GetGPUHandleByIndex(descriptor_start);
	binding_table_desc.SizeInDescriptors = descriptor_count;


	THROWIFFAILED(
		dml_dev->CreateBindingTable(
			&binding_table_desc,
			IID_PPV_ARGS(&temp_binding_table)
		), "Failed to create DML binding table");

	// Bind resources
	if (ConvLayer::initializer_temp_res_size != 0)
	{
		DML_BUFFER_BINDING buffer_binding{ ConvLayer::initializer_temp_res->buffer.Get(), 0, ConvLayer::initializer_temp_res_size };
		DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
		temp_binding_table->BindTemporaryResource(&binding_desc);
	}

	std::vector<DML_BUFFER_BINDING> buffer_bindings(layers.size());
	std::vector<DML_BINDING_DESC> binding_descs(layers.size());

	for (int i = 0; i < layers.size(); i++)
	{
		if (layers[i].GetPersistantResource() != nullptr)
		{
			// Bind as output of the initializer
			buffer_bindings[i] = { layers[i].GetPersistantResource()->buffer.Get(), 0, layers[i].GetPersistantResourceSize() };
			binding_descs[i] = { DML_BINDING_TYPE_BUFFER, &buffer_bindings[i] };
			
		}
	}
	temp_binding_table->BindOutputs(layers.size(), binding_descs.data());
	
	// Bind conv input (weight and bias)
	DML_BUFFER_BINDING empty_buffer_binding = { nullptr, 0, 0 };
	std::vector<std::array<DML_BUFFER_BINDING, 3>> conv_buffer_bindings(layers.size());
	for (int i = 0; i < layers.size(); i++)
		conv_buffer_bindings[i] = {
			empty_buffer_binding,
			{layers[i].GetWeightTensor().buffer.Get(), 0, layers[i].GetWeightTensor().buffer->GetDesc().Width},
			{layers[i].GetBiasTensor().buffer.Get(), 0, layers[i].GetBiasTensor().buffer->GetDesc().Width}};

	std::vector<DML_BUFFER_ARRAY_BINDING> conv_buffer_array_bindings(layers.size());
	for(int i = 0; i < layers.size(); i++)
		conv_buffer_array_bindings[i] = {3, conv_buffer_bindings[i].data()};

	std::vector<DML_BINDING_DESC> conv_bindings(layers.size());
	for (int i = 0; i < layers.size(); i++)
		conv_bindings[i] = { DML_BINDING_TYPE_BUFFER_ARRAY, &conv_buffer_array_bindings[i] };

	temp_binding_table->BindInputs(layers.size(), conv_bindings.data());


	command_recorder->RecordDispatch(command_list, ConvLayer::conv_initializer.Get(), temp_binding_table.Get());

	//conv_initializer.Reset();
	//initializer_temp_res.release();
	//initializer_temp_res_size = 0;
}