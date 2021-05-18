#define NOMINMAX
#include "add_layer.h"
#include "graphics/internal/egx_internal.h"

ComPtr<IDMLOperatorInitializer> egx::AddLayer::add_layer_initializer = nullptr;
ComPtr<IDMLBindingTable> egx::AddLayer::temp_binding_table = nullptr;
UINT64 egx::AddLayer::initializer_temp_res_size = 0;
UINT egx::AddLayer::descriptor_start = 0;
UINT egx::AddLayer::descriptor_count = 0;
std::unique_ptr<egx::UnorderedAccessBuffer> egx::AddLayer::initializer_temp_res = nullptr;

namespace
{
	const egx::Layout tensor_layout = egx::Layout::NHWC;
	const DML_TENSOR_DATA_TYPE tensor_type = DML_TENSOR_DATA_TYPE_FLOAT16;
}

egx::AddLayer::AddLayer(Device& dev, IDMLDevice* dml_dev, const DMLDims& input_dims)
{
	this->input_dims = { input_dims.dims[0], input_dims.dims[1], input_dims.dims[2], input_dims.dims[3] };
	output_dims = this->input_dims;

	// Describe input and output tensors
	auto input_strides = CalculateStrides(
		tensor_layout,
		{ input_dims.dims[0], input_dims.dims[1], input_dims.dims[2], input_dims.dims[3] },
		{ false, false, false, false });

	input_buffer_size = DMLCalcBufferTensorSize(tensor_type, 4, input_dims.dims, input_strides.data());
	output_buffer_size = input_buffer_size;

	DML_BUFFER_TENSOR_DESC buffer_desc = {};
	buffer_desc.DataType = tensor_type;
	buffer_desc.Flags = DML_TENSOR_FLAG_NONE;
	buffer_desc.DimensionCount = 4;
	buffer_desc.Sizes = input_dims.dims;
	buffer_desc.Strides = input_strides.data();
	buffer_desc.TotalTensorSizeInBytes = input_buffer_size;
	DML_TENSOR_DESC tensor_desc = {};
	tensor_desc.Type = DML_TENSOR_TYPE_BUFFER;
	tensor_desc.Desc = &buffer_desc;

	// Describe, create and compile conv operator
	DML_ELEMENT_WISE_ADD_OPERATOR_DESC add_desc = {};
	add_desc.ATensor = &tensor_desc;
	add_desc.BTensor = &tensor_desc;
	add_desc.OutputTensor = &tensor_desc;

	DML_OPERATOR_DESC op_desc = { DML_OPERATOR_ELEMENT_WISE_ADD, &add_desc };

	ComPtr<IDMLOperator> op;
	THROWIFFAILED(dml_dev->CreateOperator(&op_desc, IID_PPV_ARGS(&op)), "Failed to create DML addition operator");
	THROWIFFAILED(dml_dev->CompileOperator(op.Get(), DML_EXECUTION_FLAG_ALLOW_HALF_PRECISION_COMPUTATION, IID_PPV_ARGS(&compiled_op))
		, "Failed to compile DML addition operator");
	

	// Create persistant resource
	auto binding_props = compiled_op->GetBindingProperties();
	persistant_resource_size = binding_props.PersistentResourceSize;
	if (persistant_resource_size > 0)
		persistent_resource = std::make_unique<UnorderedAccessBuffer>(dev, persistant_resource_size);

	temporary_resource_size = binding_props.TemporaryResourceSize;
	if (temporary_resource_size > 0)
		temporary_resource = std::make_unique<UnorderedAccessBuffer>(dev, temporary_resource_size);
}


void egx::AddLayer::CreateBindingTable(IDMLDevice* dml_dev, DescriptorHeap& desc_heap, UINT index)
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


void egx::AddLayer::BindResources(ID3D12Resource* A, ID3D12Resource* B, ID3D12Resource* C)
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
	DML_BUFFER_BINDING A_buffer_binding{ A, 0, GetInputBufferSize() };
	DML_BUFFER_BINDING B_buffer_binding{ B, 0, GetInputBufferSize() };
	DML_BINDING_DESC input_binding_desc[] = { 
		{ DML_BINDING_TYPE_BUFFER, &A_buffer_binding },
		{ DML_BINDING_TYPE_BUFFER, &B_buffer_binding },
	};
	GetBindingTable()->BindInputs(2, input_binding_desc);

	// Bind output
	DML_BUFFER_BINDING output_buffer_binding{ C, 0, GetOutputBufferSize() };
	DML_BINDING_DESC output_binding_desc{ DML_BINDING_TYPE_BUFFER, &output_buffer_binding };
	GetBindingTable()->BindOutputs(1, &output_binding_desc);
}

void egx::AddLayer::CreateAddLayerInitializer(Device& dev, IDMLDevice* dml_dev, std::vector<AddLayer>& layers)
{
	std::vector<IDMLCompiledOperator*> comp_ops(layers.size());
	for (int i = 0; i < layers.size(); i++)
		comp_ops[i] = layers[i].GetCompiledOperator();

	THROWIFFAILED(
		dml_dev->CreateOperatorInitializer(
			comp_ops.size(),
			comp_ops.data(),
			IID_PPV_ARGS(&add_layer_initializer)
		), "Failed to create DML operator initializer");


	auto binding_props = add_layer_initializer->GetBindingProperties();
	initializer_temp_res_size = binding_props.TemporaryResourceSize;
	if (initializer_temp_res_size > 0)
		initializer_temp_res = std::make_unique<UnorderedAccessBuffer>(dev, initializer_temp_res_size);
}
int egx::AddLayer::GetDescriptorCount(std::vector<AddLayer>& layers)
{
	UINT max_desc_count = add_layer_initializer->GetBindingProperties().RequiredDescriptorCount;

	for (auto& layer : layers)
		max_desc_count = std::max(max_desc_count, layer.GetCompiledOperator()->GetBindingProperties().RequiredDescriptorCount);
	return max_desc_count;
}

void egx::AddLayer::InitializeAddLayers(IDMLDevice* dml_dev, IDMLCommandRecorder* command_recorder, ID3D12CommandList* command_list, DescriptorHeap& desc_heap, std::vector<AddLayer>& layers)
{
	// Create binding table
	ComPtr<IDMLBindingTable> temp_binding_table;
	DML_BINDING_TABLE_DESC binding_table_desc = {};
	binding_table_desc.Dispatchable = AddLayer::add_layer_initializer.Get();
	binding_table_desc.CPUDescriptorHandle = desc_heap.GetCPUHandleByIndex(descriptor_start);
	binding_table_desc.GPUDescriptorHandle = desc_heap.GetGPUHandleByIndex(descriptor_start);
	binding_table_desc.SizeInDescriptors = descriptor_count;


	THROWIFFAILED(
		dml_dev->CreateBindingTable(
			&binding_table_desc,
			IID_PPV_ARGS(&temp_binding_table)
		), "Failed to create DML binding table");

	// Bind resources
	if (AddLayer::initializer_temp_res_size != 0)
	{
		DML_BUFFER_BINDING buffer_binding{ AddLayer::initializer_temp_res->buffer.Get(), 0, AddLayer::initializer_temp_res_size };
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


	command_recorder->RecordDispatch(command_list, AddLayer::add_layer_initializer.Get(), temp_binding_table.Get());

	//conv_initializer.Reset();
	//initializer_temp_res.release();
	//initializer_temp_res_size = 0;
}