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
    DMLDims input_buffer_size = { 1, 32, 1080 / 4, 1920 / 4};
    //UINT output_channels = 128;

    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), input_buffer_size, 32, 1, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[0].GetOutputDims(), 32, 1, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[1].GetOutputDims(), 32, 1, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[2].GetOutputDims(), 256, 1, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[3].GetOutputDims(), 256, 1, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[4].GetOutputDims(), 32, 1, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[5].GetOutputDims(), 32, 1, true));

    // Create operator initializer
    ConvLayer::CreateConvInitializer(dev, dml_device.Get(), conv_layers);

    // Find size of descriptor heap
    ConvLayer::descriptor_start = 0;
    ConvLayer::descriptor_count = ConvLayer::GetDescriptorCount(conv_layers);
    UINT descriptor_count = ConvLayer::descriptor_count * conv_layers.size();

    // Create descriptor heap for DML
    descriptor_heap = std::make_unique<DescriptorHeap>(dev.device, DescriptorType::Buffer, descriptor_count);

    // Set descriptor heap
    context.SetDescriptorHeap(*descriptor_heap);


    // Create command recorder
    THROWIFFAILED(
        dml_device->CreateCommandRecorder(IID_PPV_ARGS(&command_recorder)),
        "Failed to create command recorder");

    
    ConvLayer::InitializeConvLayers(dml_device.Get(), command_recorder.Get(), context.command_list.Get(), *descriptor_heap, conv_layers);


    // Set binding table from init to execute
    conv_layers[0].CreateBindingTable(dml_device.Get(), *descriptor_heap, 0);
    conv_layers[1].CreateBindingTable(dml_device.Get(), *descriptor_heap, 1);
    conv_layers[2].CreateBindingTable(dml_device.Get(), *descriptor_heap, 2);
    conv_layers[3].CreateBindingTable(dml_device.Get(), *descriptor_heap, 3);
    conv_layers[4].CreateBindingTable(dml_device.Get(), *descriptor_heap, 4);
    conv_layers[5].CreateBindingTable(dml_device.Get(), *descriptor_heap, 5);
    conv_layers[6].CreateBindingTable(dml_device.Get(), *descriptor_heap, 6);


    // Create input and output buffer
    UINT64 max_int1_size = std::max(conv_layers[0].GetOutputBufferSize(), std::max(conv_layers[2].GetOutputBufferSize(), conv_layers[4].GetOutputBufferSize()));
    UINT64 max_int2_size = std::max(conv_layers[1].GetOutputBufferSize(), std::max(conv_layers[3].GetOutputBufferSize(), conv_layers[6].GetOutputBufferSize()));
    input_buffer = std::make_unique<UnorderedAccessBuffer>(dev, conv_layers[0].GetInputBufferSize());
    intermediate_buffer1 = std::make_unique<UnorderedAccessBuffer>(dev, max_int1_size);
    intermediate_buffer2 = std::make_unique<UnorderedAccessBuffer>(dev, max_int2_size);
    output_buffer = std::make_unique<UnorderedAccessBuffer>(dev, conv_layers[5].GetOutputBufferSize());

    conv_layers[0].BindResources(input_buffer->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[1].BindResources(intermediate_buffer1->buffer.Get(), intermediate_buffer2->buffer.Get());
    conv_layers[2].BindResources(intermediate_buffer2->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[3].BindResources(intermediate_buffer1->buffer.Get(), intermediate_buffer2->buffer.Get());
    conv_layers[4].BindResources(intermediate_buffer2->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[5].BindResources(intermediate_buffer1->buffer.Get(), intermediate_buffer2->buffer.Get());
    conv_layers[6].BindResources(intermediate_buffer2->buffer.Get(), output_buffer->buffer.Get());

}

//void egx::MasterNet::Execute(egx::Texture2D& texture_in, egx::Texture2D& texture_out)
void egx::MasterNet::Execute(egx::Device& dev, egx::CommandContext& context)
{
    context.SetDescriptorHeap(*descriptor_heap);


    // Record and execute
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[0].GetCompiledOperator(), conv_layers[0].GetBindingTable());
    context.SetUABarrier();
    //context.SetUABarrier(*intermediate_buffer1);
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[1].GetCompiledOperator(), conv_layers[1].GetBindingTable());
    context.SetUABarrier();
    //context.SetUABarrier(*intermediate_buffer2);
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[2].GetCompiledOperator(), conv_layers[2].GetBindingTable());
    context.SetUABarrier();
    //context.SetUABarrier(*intermediate_buffer1);
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[3].GetCompiledOperator(), conv_layers[3].GetBindingTable());
    context.SetUABarrier();
    //context.SetUABarrier(*intermediate_buffer2);
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[4].GetCompiledOperator(), conv_layers[4].GetBindingTable());
    context.SetUABarrier();
    //context.SetUABarrier(*intermediate_buffer1);
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[5].GetCompiledOperator(), conv_layers[5].GetBindingTable());
    context.SetUABarrier();
    //context.SetUABarrier(*intermediate_buffer2);
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[6].GetCompiledOperator(), conv_layers[6].GetBindingTable());

    //dev.QueueListAndWaitForFinish(context);

    context.SetDescriptorHeap(*dev.buffer_heap);
}