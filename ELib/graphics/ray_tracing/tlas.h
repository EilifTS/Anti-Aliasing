#pragma once
#include <vector>
#include <memory>
#include "../internal/egx_common.h"
#include "../model.h"

namespace egx
{
	class TLAS
	{
	public:
		TLAS() : srv_cpu(), srv_gpu() {};

		void Build(Device& dev, CommandContext& context, std::vector<std::shared_ptr<Model>>& models);
		void ReBuild(CommandContext& context, std::vector<std::shared_ptr<Model>>& models);

	private:
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC as_desc;

		std::unique_ptr<UploadHeap> instances_buffer;
		std::unique_ptr<GPUBuffer> scratch_buffer;
		std::unique_ptr<GPUBuffer> result_buffer;

		D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu;

	private:
		friend ShaderTable;
	};

}