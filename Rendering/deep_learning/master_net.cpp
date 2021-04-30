#define NOMINMAX
#include <fstream>
#include "master_net.h"
#include "graphics/internal/egx_internal.h"
#include "graphics/internal/d3dx12.h"
#include "float16_compressor.h"

namespace
{
    
}

egx::MasterNet::MasterNet(Device& dev, CommandContext& context, const ema::point2D& window_size, int upsample_factor)
    : window_size(window_size), upsample_factor(upsample_factor)
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

    // Load weights
    auto weight_map = loadWeights("../network/MasterNet4x4/nn_weights.bin");
    if(upsample_factor == 2)
        weight_map = loadWeights("../network/MasterNet2x2/nn_weights.bin");

    // Create layers
    DMLDims input_buffer_size = { 1, 8, window_size.y, window_size.x};
    //UINT output_channels = 128;

    // Down
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), input_buffer_size, 32, 4, false, 4));

    // Res block 1
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[0].GetOutputDims(), 32, 3, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[1].GetOutputDims(), 32, 3, false));
    add_layers.push_back(AddLayer(dev, dml_device.Get(), conv_layers[2].GetOutputDims()));

    // Res block 2
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), add_layers[0].GetOutputDims(), 32, 3, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[3].GetOutputDims(), 32, 3, false));
    add_layers.push_back(AddLayer(dev, dml_device.Get(), conv_layers[4].GetOutputDims()));

    // Res block 3
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), add_layers[1].GetOutputDims(), 32, 3, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[5].GetOutputDims(), 32, 3, false));
    add_layers.push_back(AddLayer(dev, dml_device.Get(), conv_layers[6].GetOutputDims()));

    // Res block 4
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), add_layers[2].GetOutputDims(), 32, 3, true));
    conv_layers.push_back(ConvLayer(dev, dml_device.Get(), conv_layers[7].GetOutputDims(), 32, 3, false));
    add_layers.push_back(AddLayer(dev, dml_device.Get(), conv_layers[8].GetOutputDims()));

    // Up
    shuffle_layers.push_back(PixelShuffle(dev, dml_device.Get(), add_layers[3].GetOutputDims(), 4));

    // Upload weights as biases
    // Down
    conv_layers[0].UploadWeights(dev, context, weight_map["down.0.weight"]);
    conv_layers[0].UploadBias(dev, context, weight_map["down.0.bias"]);

    // Res block 1
    conv_layers[1].UploadWeights(dev, context, weight_map["cnn1.0.weight"]);
    conv_layers[1].UploadBias(dev, context, weight_map["cnn1.0.bias"]);
    conv_layers[2].UploadWeights(dev, context, weight_map["cnn1.2.weight"]);
    conv_layers[2].UploadBias(dev, context, weight_map["cnn1.2.bias"]);

    // Res block 2
    conv_layers[3].UploadWeights(dev, context, weight_map["cnn2.0.weight"]);
    conv_layers[3].UploadBias(dev, context, weight_map["cnn2.0.bias"]);
    conv_layers[4].UploadWeights(dev, context, weight_map["cnn2.2.weight"]);
    conv_layers[4].UploadBias(dev, context, weight_map["cnn2.2.bias"]);

    // Res block 3
    conv_layers[5].UploadWeights(dev, context, weight_map["cnn3.0.weight"]);
    conv_layers[5].UploadBias(dev, context, weight_map["cnn3.0.bias"]);
    conv_layers[6].UploadWeights(dev, context, weight_map["cnn3.2.weight"]);
    conv_layers[6].UploadBias(dev, context, weight_map["cnn3.2.bias"]);

    // Res block 4
    conv_layers[7].UploadWeights(dev, context, weight_map["cnn4.0.weight"]);
    conv_layers[7].UploadBias(dev, context, weight_map["cnn4.0.bias"]);
    conv_layers[8].UploadWeights(dev, context, weight_map["cnn4.2.weight"]);
    conv_layers[8].UploadBias(dev, context, weight_map["cnn4.2.bias"]);

    dev.QueueList(context);
    dev.WaitForGPU();

    // Create operator initializer
    ConvLayer::CreateConvInitializer(dev, dml_device.Get(), conv_layers);
    PixelShuffle::CreatePixelShuffleInitializer(dev, dml_device.Get(), shuffle_layers);
    AddLayer::CreateAddLayerInitializer(dev, dml_device.Get(), add_layers);

    // Find size of descriptor heap
    ConvLayer::descriptor_start = 0;
    ConvLayer::descriptor_count = ConvLayer::GetDescriptorCount(conv_layers);
    UINT descriptor_count = ConvLayer::descriptor_count * conv_layers.size();
    PixelShuffle::descriptor_start = descriptor_count;
    PixelShuffle::descriptor_count = PixelShuffle::GetDescriptorCount(shuffle_layers);
    descriptor_count = descriptor_count + PixelShuffle::descriptor_count * shuffle_layers.size();
    AddLayer::descriptor_start = descriptor_count;
    AddLayer::descriptor_count = AddLayer::GetDescriptorCount(add_layers);
    descriptor_count = descriptor_count + AddLayer::descriptor_count * add_layers.size();

    // Create descriptor heap for DML
    descriptor_heap = std::make_unique<DescriptorHeap>(dev.device, DescriptorType::Buffer, descriptor_count);

    // Set descriptor heap
    context.SetDescriptorHeap(*descriptor_heap);


    // Create command recorder
    THROWIFFAILED(
        dml_device->CreateCommandRecorder(IID_PPV_ARGS(&command_recorder)),
        "Failed to create command recorder");

    
    ConvLayer::InitializeConvLayers(dml_device.Get(), command_recorder.Get(), context.command_list.Get(), *descriptor_heap, conv_layers);
    PixelShuffle::InitializePixelShuffleLayers(dml_device.Get(), command_recorder.Get(), context.command_list.Get(), *descriptor_heap, shuffle_layers);
    AddLayer::InitializeAddLayers(dml_device.Get(), command_recorder.Get(), context.command_list.Get(), *descriptor_heap, add_layers);


    // Set binding table from init to execute
    conv_layers[0].CreateBindingTable(dml_device.Get(), *descriptor_heap, 0);
    conv_layers[1].CreateBindingTable(dml_device.Get(), *descriptor_heap, 1);
    conv_layers[2].CreateBindingTable(dml_device.Get(), *descriptor_heap, 2);
    conv_layers[3].CreateBindingTable(dml_device.Get(), *descriptor_heap, 3);
    conv_layers[4].CreateBindingTable(dml_device.Get(), *descriptor_heap, 4);
    conv_layers[5].CreateBindingTable(dml_device.Get(), *descriptor_heap, 5);
    conv_layers[6].CreateBindingTable(dml_device.Get(), *descriptor_heap, 6);
    conv_layers[7].CreateBindingTable(dml_device.Get(), *descriptor_heap, 7);
    conv_layers[8].CreateBindingTable(dml_device.Get(), *descriptor_heap, 8);
    shuffle_layers[0].CreateBindingTable(dml_device.Get(), *descriptor_heap, 0);
    add_layers[0].CreateBindingTable(dml_device.Get(), *descriptor_heap, 0);
    add_layers[1].CreateBindingTable(dml_device.Get(), *descriptor_heap, 1);
    add_layers[2].CreateBindingTable(dml_device.Get(), *descriptor_heap, 2);
    add_layers[3].CreateBindingTable(dml_device.Get(), *descriptor_heap, 3);

    // Create input and output buffer
    UINT64 max_int1_size = shuffle_layers[0].GetOutputBufferSize();
    UINT64 max_int2_size = conv_layers[0].GetOutputBufferSize();
    UINT64 max_int3_size = add_layers[0].GetOutputBufferSize();
    input_buffer = std::make_unique<UnorderedAccessBuffer>(dev, conv_layers[0].GetInputBufferSize());
    intermediate_buffer1 = std::make_unique<UnorderedAccessBuffer>(dev, max_int1_size);
    intermediate_buffer2 = std::make_unique<UnorderedAccessBuffer>(dev, max_int2_size);
    intermediate_buffer3 = std::make_unique<UnorderedAccessBuffer>(dev, max_int3_size);
    output_buffer = std::make_unique<UnorderedAccessBuffer>(dev, shuffle_layers[0].GetOutputBufferSize());

    input_buffer->CreateUnorderedAccessView(dev, true);
    output_buffer->CreateUnorderedAccessView(dev, true);
    input_buffer->CreateShaderResourceView(dev);
    output_buffer->CreateShaderResourceView(dev);

    // Down
    //shuffle_layers[0].BindResources(input_buffer->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[0].BindResources(input_buffer->buffer.Get(), intermediate_buffer3->buffer.Get());

    // Res block 1
    conv_layers[1].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[2].BindResources(intermediate_buffer1->buffer.Get(), intermediate_buffer2->buffer.Get());
    add_layers[0].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer2->buffer.Get());

    // Res block 2
    conv_layers[3].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[4].BindResources(intermediate_buffer1->buffer.Get(), intermediate_buffer2->buffer.Get());
    add_layers[1].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer2->buffer.Get());

    // Res block 3
    conv_layers[5].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[6].BindResources(intermediate_buffer1->buffer.Get(), intermediate_buffer2->buffer.Get());
    add_layers[2].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer2->buffer.Get());

    // Res block 4
    conv_layers[7].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer1->buffer.Get());
    conv_layers[8].BindResources(intermediate_buffer1->buffer.Get(), intermediate_buffer2->buffer.Get());
    add_layers[3].BindResources(intermediate_buffer3->buffer.Get(), intermediate_buffer2->buffer.Get());

    shuffle_layers[0].BindResources(intermediate_buffer3->buffer.Get(), output_buffer->buffer.Get());

    dev.QueueList(context);
    dev.WaitForGPU();
}

//void egx::MasterNet::Execute(egx::Texture2D& texture_in, egx::Texture2D& texture_out)
void egx::MasterNet::Execute(egx::Device& dev, 
    egx::CommandContext& context)
{
    context.SetDescriptorHeap(*descriptor_heap);

    context.SetTransitionBuffer(GetInputBuffer(), egx::GPUBufferState::UnorderedAccess);
    context.SetTransitionBuffer(GetOutputBuffer(), egx::GPUBufferState::UnorderedAccess);

    // Record and execute

    // Down
    //command_recorder->RecordDispatch(context.command_list.Get(), shuffle_layers[0].GetCompiledOperator(), shuffle_layers[0].GetBindingTable());
    //context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[0].GetCompiledOperator(), conv_layers[0].GetBindingTable());
    context.SetUABarrier();

    // Res block 1
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[1].GetCompiledOperator(), conv_layers[1].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[2].GetCompiledOperator(), conv_layers[2].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), add_layers[0].GetCompiledOperator(), add_layers[0].GetBindingTable());
    context.SetUABarrier();

    // Res block 2
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[3].GetCompiledOperator(), conv_layers[3].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[4].GetCompiledOperator(), conv_layers[4].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), add_layers[1].GetCompiledOperator(), add_layers[1].GetBindingTable());
    context.SetUABarrier();

    // Res block 3
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[5].GetCompiledOperator(), conv_layers[5].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[6].GetCompiledOperator(), conv_layers[6].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), add_layers[2].GetCompiledOperator(), add_layers[2].GetBindingTable());
    context.SetUABarrier();

    // Res block 4
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[7].GetCompiledOperator(), conv_layers[7].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), conv_layers[8].GetCompiledOperator(), conv_layers[8].GetBindingTable());
    context.SetUABarrier();
    command_recorder->RecordDispatch(context.command_list.Get(), add_layers[3].GetCompiledOperator(), add_layers[3].GetBindingTable());
    context.SetUABarrier();

    // Up
    command_recorder->RecordDispatch(context.command_list.Get(), shuffle_layers[0].GetCompiledOperator(), shuffle_layers[0].GetBindingTable());
    context.SetUABarrier();

    //dev.QueueListAndWaitForFinish(context);

    context.SetDescriptorHeap(*dev.buffer_heap);
}


std::unordered_map<std::string, std::vector<uint16_t>> egx::MasterNet::loadWeights(const std::string& file_path)
{
    std::ifstream file(file_path, std::ios::binary);
    if (file.fail())
        throw std::runtime_error("Failed to load file " + file_path);

    UINT num_weights = 0;
    file.read(reinterpret_cast<char*>(&num_weights), 4);

    std::unordered_map<std::string, std::vector<uint16_t>> output;
    for (UINT i = 0; i < num_weights; i++)
    {
        UINT name_length = 0;
        char name[255] = {};
        file.read(reinterpret_cast<char*>(&name_length), 4);
        file.read(name, name_length);

        UINT float_count = 0;
        file.read(reinterpret_cast<char*>(&float_count), 4);
        std::vector<float> weights(float_count);
        std::vector<uint16_t> weights16(float_count);
        file.read(reinterpret_cast<char*>(weights.data()), float_count * sizeof(float));

        // Compress floats
        for (int j = 0; j < (int)weights.size(); j++)
            weights16[j] = Float16Compressor::compress(weights[j]);

        output[std::string(name)] = weights16;
    }
    return output;
}