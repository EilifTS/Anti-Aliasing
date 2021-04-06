#pragma once
#include <vector>
#include <memory>
#include "dml_common.h"
#include "graphics/device.h"
#include "graphics/command_context.h"
#include "graphics/unordered_access_buffer.h"

#include "conv_layer.h"
#include "pixel_shuffle.h"
#include "add_layer.h"

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

		std::unique_ptr<UnorderedAccessBuffer> input_buffer;
		std::unique_ptr<UnorderedAccessBuffer> output_buffer;
		std::unique_ptr<UnorderedAccessBuffer> intermediate_buffer1;
		std::unique_ptr<UnorderedAccessBuffer> intermediate_buffer2;
		std::unique_ptr<UnorderedAccessBuffer> intermediate_buffer3;

		std::vector<ConvLayer> conv_layers;
		std::vector<PixelShuffle> shuffle_layers;
		std::vector<AddLayer> add_layers;

		UINT64 tensor_buffer_size;



	};
}