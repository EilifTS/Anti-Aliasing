#define NOMINMAX
#include "master_net.h"
#include "graphics/internal/egx_internal.h"
#include "graphics/internal/d3dx12.h"

namespace
{
    
}

egx::MasterNet::MasterNet(egx::Device& dev, egx::CommandContext& context)
{
	// Create DML device
	DML_CREATE_DEVICE_FLAGS dml_device_flags = DML_CREATE_DEVICE_FLAG_NONE;

	// Enable debugging
#if defined (_DEBUG)
	dml_device_flags |= DML_CREATE_DEVICE_FLAG_DEBUG;
#endif

	THROWIFFAILED(
		DMLCreateDevice(
			dev.device.Get(),
			dml_device_flags,
			IID_PPV_ARGS(&dml_device)
		),
		"Failed to create DML device"
	);

    // Create layers
    DMLDims input_buffer_size = { 1, 3, 1080, 1920 };
    UINT output_channels = 3;

    conv_layer1 = std::make_unique<ConvLayer>(dev, dml_device.Get(), input_buffer_size, output_channels);

    tensor_buffer_size = conv_layer1->GetInputBufferSize();

    // Create operator initializer
    ComPtr<IDMLOperatorInitializer> op_initializer;
    IDMLCompiledOperator* compiled_ops[] = { conv_layer1->GetCompiledOperator() };
    THROWIFFAILED(
        dml_device->CreateOperatorInitializer(
            ARRAYSIZE(compiled_ops),
            compiled_ops,
            IID_PPV_ARGS(&op_initializer)
        ), "Failed to create DML operator initializer");

    // Find size of descriptor heap
    DML_BINDING_PROPERTIES init_bind_props = op_initializer->GetBindingProperties();
    DML_BINDING_PROPERTIES exec_bind_props = conv_layer1->GetCompiledOperator()->GetBindingProperties();
    UINT descriptor_count = std::max(init_bind_props.RequiredDescriptorCount, exec_bind_props.RequiredDescriptorCount);


    // Create descriptor heap for DML
    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
    descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptor_heap_desc.NumDescriptors = descriptor_count;
    descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    THROWIFFAILED(
        dev.device->CreateDescriptorHeap(
            &descriptor_heap_desc,
            IID_PPV_ARGS(&descriptor_heap)
        ), "Failed to create DML descriptor heap");

    // Set descriptor heap
    ID3D12DescriptorHeap* descriptor_heaps[] = { descriptor_heap.Get() };
    context.command_list->SetDescriptorHeaps(ARRAYSIZE(descriptor_heaps), descriptor_heaps);

    // Create binding table over the descriptor heap
    DML_BINDING_TABLE_DESC binding_table_desc = {};
    binding_table_desc.Dispatchable = op_initializer.Get();
    binding_table_desc.CPUDescriptorHandle = descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    binding_table_desc.GPUDescriptorHandle = descriptor_heap->GetGPUDescriptorHandleForHeapStart();
    binding_table_desc.SizeInDescriptors = descriptor_count;

    THROWIFFAILED(
        dml_device->CreateBindingTable(
            &binding_table_desc,
            IID_PPV_ARGS(&binding_table)
        ), "Failed to create DML binding table");

    // Create temp and persistent resources
    UINT64 temp_res_size = std::max(init_bind_props.TemporaryResourceSize, exec_bind_props.TemporaryResourceSize);
    UINT64 pers_res_size = exec_bind_props.PersistentResourceSize;

    D3D12_HEAP_PROPERTIES default_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    if(temp_res_size != 0)
    {
        D3D12_RESOURCE_DESC pers_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(temp_res_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        THROWIFFAILED(
            dev.device->CreateCommittedResource(
                &default_heap_prop,
                D3D12_HEAP_FLAG_NONE,
                &pers_buffer_desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&temp_buffer)
            ), "Failed to create DML temp res");

        // Bind init resource
        if (init_bind_props.TemporaryResourceSize != 0)
        {
            DML_BUFFER_BINDING buffer_binding{ temp_buffer.Get(), 0, temp_res_size };
            DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
            binding_table->BindTemporaryResource(&binding_desc);
        }
    }

    if (pers_res_size != 0)
    {
        D3D12_RESOURCE_DESC pers_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(pers_res_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        THROWIFFAILED(
            dev.device->CreateCommittedResource(
                &default_heap_prop,
                D3D12_HEAP_FLAG_NONE,
                &pers_buffer_desc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr,
                IID_PPV_ARGS(&pers_buffer)
            ), "Failed to create DML temp res");

        // Bind as output of the initializer
        DML_BUFFER_BINDING buffer_binding{ pers_buffer.Get(), 0, pers_res_size };
        DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
        binding_table->BindOutputs(1, &binding_desc);
    }

    // Create command recorder
    THROWIFFAILED(
        dml_device->CreateCommandRecorder(IID_PPV_ARGS(&command_recorder)),
        "Failed to create command recorder");

    // Bind conv input (weight and bias)
    DML_BUFFER_BINDING empty_buffer_binding = { nullptr, 0, 0 };
    DML_BUFFER_BINDING conv_buffer_bindings[][3] =
    {
        {
            empty_buffer_binding,
            {conv_layer1->GetWeightTensor().buffer.Get(), 0, conv_layer1->GetWeightTensor().buffer->GetDesc().Width},
            {conv_layer1->GetBiasTensor().buffer.Get(), 0, conv_layer1->GetBiasTensor().buffer->GetDesc().Width},
        }
    };
    DML_BUFFER_ARRAY_BINDING conv_buffer_array_bindings[] = { {3, conv_buffer_bindings[0]} };

    DML_BINDING_DESC conv_bindings[] = { {DML_BINDING_TYPE_BUFFER_ARRAY, &conv_buffer_array_bindings[0]} };

    binding_table->BindInputs(1, conv_bindings);

    // Record init command
    command_recorder->RecordDispatch(context.command_list.Get(), op_initializer.Get(), binding_table.Get());

    //dev.QueueListAndWaitForFinish(context);

    // Execute
    //context.command_list->SetDescriptorHeaps(ARRAYSIZE(descriptor_heaps), descriptor_heaps);

    // Set binding table from init to execute
    binding_table_desc.Dispatchable = conv_layer1->GetCompiledOperator();

    THROWIFFAILED(binding_table->Reset(&binding_table_desc), "Failed to reset DML binding table");

    if (temp_res_size != 0)
    {
        DML_BUFFER_BINDING buffer_binding{ temp_buffer.Get(), 0, temp_res_size };
        DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
        binding_table->BindTemporaryResource(&binding_desc);
    }
    if (pers_res_size != 0)
    {
        DML_BUFFER_BINDING buffer_binding{ pers_buffer.Get(), 0, pers_res_size };
        DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
        binding_table->BindPersistentResource(&binding_desc);
    }

    // Create input buffer
    D3D12_RESOURCE_DESC input_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(tensor_buffer_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    THROWIFFAILED(
        dev.device->CreateCommittedResource(
            &default_heap_prop,
            D3D12_HEAP_FLAG_NONE,
            &input_buffer_desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&input_buffer)
        ), "Failed to create DML input buffer");

    // Bind input
    DML_BINDING_DESC empty_binding_desc = { DML_BINDING_TYPE_NONE, nullptr };
    DML_BUFFER_BINDING input_buffer_binding{ input_buffer.Get(), 0, tensor_buffer_size };
    DML_BINDING_DESC input_binding_desc[] = { { DML_BINDING_TYPE_BUFFER, &input_buffer_binding }, empty_binding_desc, empty_binding_desc };
    binding_table->BindInputs(3, input_binding_desc);

    // Create output buffer
    D3D12_RESOURCE_DESC output_buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(tensor_buffer_size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    THROWIFFAILED(
        dev.device->CreateCommittedResource(
            &default_heap_prop,
            D3D12_HEAP_FLAG_NONE,
            &output_buffer_desc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&output_buffer)
        ), "Failed to create DML output buffer");

    

}

//void egx::MasterNet::Execute(egx::Texture2D& texture_in, egx::Texture2D& texture_out)
void egx::MasterNet::Execute(egx::Device& dev, egx::CommandContext& context)
{
    ID3D12DescriptorHeap* descriptor_heaps[] = { descriptor_heap.Get() };
    context.command_list->SetDescriptorHeaps(ARRAYSIZE(descriptor_heaps), descriptor_heaps);

    // Bind output
    DML_BUFFER_BINDING output_buffer_binding{ output_buffer.Get(), 0, tensor_buffer_size };
    DML_BINDING_DESC output_binding_desc{ DML_BINDING_TYPE_BUFFER, &output_buffer_binding };
    binding_table->BindOutputs(1, &output_binding_desc);

    // Record and execute
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layer1->GetCompiledOperator(), binding_table.Get());

    //dev.QueueListAndWaitForFinish(context);

    context.SetDescriptorHeap(*dev.buffer_heap);
}