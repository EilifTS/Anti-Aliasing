#pragma once
#include <vector>
#include <memory>
#include "dml_common.h"
#include "graphics/device.h"
#include "graphics/command_context.h"

#include "conv_layer.h"

namespace egx
{
	class MasterNet
	{
	public:
		MasterNet(egx::Device& dev, egx::CommandContext& context);

		//void Execute(egx::Texture2D& texture_in, egx::Texture2D& texture_out);
		void Execute(egx::Device& dev, egx::CommandContext& context);
	private:
		ComPtr<IDMLDevice> dml_device;
		std::unique_ptr<DescriptorHeap> descriptor_heap;
		ComPtr<IDMLCommandRecorder> command_recorder;
		ComPtr<ID3D12Resource> input_buffer;
		ComPtr<ID3D12Resource> output_buffer;

		std::vector<ConvLayer> conv_layers;

		UINT64 tensor_buffer_size;



	};
}