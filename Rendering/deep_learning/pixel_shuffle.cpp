#define NOMINMAX
#include "pixel_shuffle.h"
#include "graphics/internal/egx_internal.h"

ComPtr<IDMLOperatorInitializer> egx::PixelShuffle::pixel_shuffle_initializer = nullptr;
ComPtr<IDMLBindingTable> egx::PixelShuffle::temp_binding_table = nullptr;
UINT64 egx::PixelShuffle::initializer_temp_res_size = 0;
UINT egx::PixelShuffle::descriptor_start = 0;
UINT egx::PixelShuffle::descriptor_count = 0;
std::unique_ptr<egx::UnorderedAccessBuffer> egx::PixelShuffle::initializer_temp_res = nullptr;

namespace
{
	const egx::Layout tensor_layout = egx::Layout::NHWC;
	const DML_TENSOR_DATA_TYPE tensor_type = DML_TENSOR_DATA_TYPE_FLOAT16;
}

egx::PixelShuffle::PixelShuffle(Device& dev, IDMLDevice* dml_dev, const DMLDims& input_dims, UINT block_size, bool inverse)
{
	this->input_dims = { input_dims.dims[0], input_dims.dims[1], input_dims.dims[2], input_dims.dims[3] };
	if(!inverse)
		output_dims = { input_dims.dims[0], input_dims.dims[1] / (block_size * block_size), input_dims.dims[2] * block_size, input_dims.dims[3] * block_size };
	else
		output_dims = { input_dims.dims[0], input_dims.dims[1] * (block_size * block_size), input_dims.dims[2] / block_size, input_dims.dims[3] / block_size };

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

	
	// Describe, create and compile conv operator
	if (!inverse)
	{
		DML_DEPTH_TO_SPACE_OPERATOR_DESC dts_desc = {};
		dts_desc.InputTensor = &input_desc;
		dts_desc.OutputTensor = &output_desc;
		dts_desc.BlockSize = block_size;

		DML_OPERATOR_DESC op_desc = { DML_OPERATOR_DEPTH_TO_SPACE, &dts_desc };

		ComPtr<IDMLOperator> op;
		THROWIFFAILED(dml_dev->CreateOperator(&op_desc, IID_PPV_ARGS(&op)), "Failed to create DML depth to space operator");
		THROWIFFAILED(dml_dev->CompileOperator(op.Get(), DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION, IID_PPV_ARGS(&compiled_op))
			, "Failed to compile DML depth to space operator");
	}
	else
	{
		DML_SPACE_TO_DEPTH_OPERATOR_DESC std_desc = {};
		std_desc.InputTensor = &input_desc;
		std_desc.OutputTensor = &output_desc;
		std_desc.BlockSize = block_size;

		DML_OPERATOR_DESC op_desc = { DML_OPERATOR_SPACE_TO_DEPTH, &std_desc };

		ComPtr<IDMLOperator> op;
		THROWIFFAILED(dml_dev->CreateOperator(&op_desc, IID_PPV_ARGS(&op)), "Failed to create DML space to depth operator");
		THROWIFFAILED(dml_dev->CompileOperator(op.Get(), DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION, IID_PPV_ARGS(&compiled_op))
			, "Failed to compile DML space to depth operator");
	}

	// Create persistant resource
	auto binding_props = compiled_op->GetBindingProperties();
	persistant_resource_size = binding_props.PersistentResourceSize;
	if (persistant_resource_size > 0)
		persistent_resource = std::make_unique<UnorderedAccessBuffer>(dev, persistant_resource_size);

	temporary_resource_size = binding_props.TemporaryResourceSize;
	if (temporary_resource_size > 0)
		temporary_resource = std::make_unique<UnorderedAccessBuffer>(dev, temporary_resource_size);
}


void egx::PixelShuffle::CreateBindingTable(IDMLDevice* dml_dev, DescriptorHeap& desc_heap, UINT index)
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


void egx::PixelShuffle::BindResources(ID3D12Resource* input, ID3D12Resource* output)
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
	DML_BUFFER_BINDING input_buffer_binding{ input, 0, GetInputBufferSize() };
	DML_BINDING_DESC input_binding_desc[] = { { DML_BINDING_TYPE_BUFFER, &input_buffer_binding } };
	GetBindingTable()->BindInputs(1, input_binding_desc);

	// Bind output
	DML_BUFFER_BINDING output_buffer_binding{ output, 0, GetOutputBufferSize() };
	DML_BINDING_DESC output_binding_desc{ DML_BINDING_TYPE_BUFFER, &output_buffer_binding };
	GetBindingTable()->BindOutputs(1, &output_binding_desc);
}

void egx::PixelShuffle::CreatePixelShuffleInitializer(Device& dev, IDMLDevice* dml_dev, std::vector<PixelShuffle>& layers)
{
	std::vector<IDMLCompiledOperator*> comp_ops(layers.size());
	for (int i = 0; i < layers.size(); i++)
		comp_ops[i] = layers[i].GetCompiledOperator();

	THROWIFFAILED(
		dml_dev->CreateOperatorInitializer(
			comp_ops.size(),
			comp_ops.data(),
			IID_PPV_ARGS(&pixel_shuffle_initializer)
		), "Failed to create DML operator initializer");


	auto binding_props = pixel_shuffle_initializer->GetBindingProperties();
	initializer_temp_res_size = binding_props.TemporaryResourceSize;
	if (initializer_temp_res_size > 0)
		initializer_temp_res = std::make_unique<UnorderedAccessBuffer>(dev, initializer_temp_res_size);
}
int egx::PixelShuffle::GetDescriptorCount(std::vector<PixelShuffle>& layers)
{
	UINT max_desc_count = pixel_shuffle_initializer->GetBindingProperties().RequiredDescriptorCount;

	for (auto& layer : layers)
		max_desc_count = std::max(max_desc_count, layer.GetCompiledOperator()->GetBindingProperties().RequiredDescriptorCount);
	return max_desc_count;
}

void egx::PixelShuffle::InitializePixelShuffleLayers(IDMLDevice* dml_dev, IDMLCommandRecorder* command_recorder, ID3D12CommandList* command_list, DescriptorHeap& desc_heap, std::vector<PixelShuffle>& layers)
{
	// Create binding table
	ComPtr<IDMLBindingTable> temp_binding_table;
	DML_BINDING_TABLE_DESC binding_table_desc = {};
	binding_table_desc.Dispatchable = PixelShuffle::pixel_shuffle_initializer.Get();
	binding_table_desc.CPUDescriptorHandle = desc_heap.GetCPUHandleByIndex(descriptor_start);
	binding_table_desc.GPUDescriptorHandle = desc_heap.GetGPUHandleByIndex(descriptor_start);
	binding_table_desc.SizeInDescriptors = descriptor_count;


	THROWIFFAILED(
		dml_dev->CreateBindingTable(
			&binding_table_desc,
			IID_PPV_ARGS(&temp_binding_table)
		), "Failed to create DML binding table");

	// Bind resources
	if (PixelShuffle::initializer_temp_res_size != 0)
	{
		DML_BUFFER_BINDING buffer_binding{ PixelShuffle::initializer_temp_res->buffer.Get(), 0, PixelShuffle::initializer_temp_res_size };
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


	temp_binding_table->BindInputs(0, nullptr);


	command_recorder->RecordDispatch(command_list, PixelShuffle::pixel_shuffle_initializer.Get(), temp_binding_table.Get());

	//conv_initializer.Reset();
	//initializer_temp_res.release();
	//initializer_temp_res_size = 0;
}