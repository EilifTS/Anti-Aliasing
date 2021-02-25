#pragma once
#pragma comment(lib, "DirectML.lib")

#include <DirectML.h>
#include "graphics/device.h"
#include "graphics/command_context.h"

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
		ComPtr<IDMLCompiledOperator> compiled_op;
		ComPtr<ID3D12DescriptorHeap> descriptor_heap;
		ComPtr<IDMLBindingTable> binding_table;
		ComPtr<ID3D12Resource> temp_buffer;
		ComPtr<ID3D12Resource> pers_buffer;
		ComPtr<IDMLCommandRecorder> command_recorder;
		ComPtr<ID3D12Resource> input_buffer;
		ComPtr<ID3D12Resource> output_buffer;

		UINT64 tensor_buffer_size;

	};
}