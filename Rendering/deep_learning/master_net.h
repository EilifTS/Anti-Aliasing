#pragma once
#include <vector>
#include <unordered_map>
#include <memory>
#include <stdint.h>
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
		MasterNet(Device& dev, CommandContext& context, const ema::point2D& window_size, int upsample_factor);

		UnorderedAccessBuffer& GetInputBuffer() { return *input_buffer; };
		UnorderedAccessBuffer& GetOutputBuffer() { return *output_buffer; };

		//void Execute(egx::Texture2D& texture_in, egx::Texture2D& texture_out);
		void Execute(Device& dev, 
			CommandContext& context);

	private:
		std::unordered_map<std::string, std::vector<uint16_t>> loadWeights(const std::string& file_path);
			
	private:
		ema::point2D window_size;
		int upsample_factor;

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