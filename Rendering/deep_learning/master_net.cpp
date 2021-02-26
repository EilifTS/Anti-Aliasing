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

    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), input_buffer_size, output_channels));

    tensor_buffer_size = conv_layers[0].GetInputBufferSize();

    // Create operator initializer
    ConvLayer::CreateConvInitializer(dev, dml_device.Get(), conv_layers);

    // Find size of descriptor heap
    UINT descriptor_count = ConvLayer::GetDescriptorCount(conv_layers);
    ConvLayer::descriptor_start = 0;
    ConvLayer::descriptor_count = descriptor_count;

    // Create descriptor heap for DML
    descriptor_heap = std::make_unique<DescriptorHeap>(dev.device, DescriptorType::Buffer, descriptor_count);

    // Set descriptor heap
    context.SetDescriptorHeap(*descriptor_heap);

    // Create binding table over the descriptor heap
    


    // Create temp and persistent resources
    

    // Create command recorder
    THROWIFFAILED(
        dml_device->CreateCommandRecorder(IID_PPV_ARGS(&command_recorder)),
        "Failed to create command recorder");

    
    ConvLayer::InitializeConvLayers(dml_device.Get(), command_recorder.Get(), context.command_list.Get(), *descriptor_heap, conv_layers);
    // Record init command

    //dev.QueueListAndWaitForFinish(context);

    // Execute
    //context.command_list->SetDescriptorHeaps(ARRAYSIZE(descriptor_heaps), descriptor_heaps);

    // Set binding table from init to execute
    conv_layers[0].CreateBindingTable(dml_device.Get(), *descriptor_heap);

    if (conv_layers[0].GetTemporaryResourceSize() != 0)
    {
        DML_BUFFER_BINDING buffer_binding{ conv_layers[0].GetTemporaryResource()->buffer.Get(), 0, conv_layers[0].GetTemporaryResourceSize() };
        DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
        conv_layers[0].GetBindingTable()->BindTemporaryResource(&binding_desc);
    }
    if (conv_layers[0].GetPersistantResourceSize() != 0)
    {
        DML_BUFFER_BINDING buffer_binding{ conv_layers[0].GetPersistantResource()->buffer.Get(), 0, conv_layers[0].GetPersistantResourceSize() };
        DML_BINDING_DESC binding_desc{ DML_BINDING_TYPE_BUFFER, &buffer_binding };
        conv_layers[0].GetBindingTable()->BindPersistentResource(&binding_desc);
    }

    // Create input buffer
    D3D12_HEAP_PROPERTIES default_heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
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
    conv_layers[0].GetBindingTable()->BindInputs(3, input_binding_desc);

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


    // Bind output
    DML_BUFFER_BINDING output_buffer_binding{ output_buffer.Get(), 0, tensor_buffer_size };
    DML_BINDING_DESC output_binding_desc{ DML_BINDING_TYPE_BUFFER, &output_buffer_binding };
    conv_layers[0].GetBindingTable()->BindOutputs(1, &output_binding_desc);

}

//void egx::MasterNet::Execute(egx::Texture2D& texture_in, egx::Texture2D& texture_out)
void egx::MasterNet::Execute(egx::Device& dev, egx::CommandContext& context)
{
    context.SetDescriptorHeap(*descriptor_heap);


    // Record and execute
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[0].GetCompiledOperator(), conv_layers[0].GetBindingTable());

    //dev.QueueListAndWaitForFinish(context);

    context.SetDescriptorHeap(*dev.buffer_heap);
}