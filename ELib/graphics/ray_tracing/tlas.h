#pragma once
#include <vector>
#include <memory>
#include "internal/egx_common.h"
#include "model.h"

namespace egx
{
	class TLAS
	{
	public:
		TLAS() {};

		void Build(Device& dev, CommandContext& context, std::vector<std::shared_ptr<Model>>& models);

	private:
		std::unique_ptr<UploadHeap> instances_buffer;
		std::unique_ptr<GPUBuffer> scratch_buffer;
		std::unique_ptr<GPUBuffer> result_buffer;

	};

}