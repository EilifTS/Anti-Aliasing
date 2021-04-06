#pragma once
#include "internal/egx_common.h"
#include "internal/gpu_buffer.h"
#include "../math/point2d.h"

namespace egx
{
	class UnorderedAccessBuffer : public GPUBuffer
	{
	public:
		UnorderedAccessBuffer(Device& dev, TextureFormat format, const ema::point2D& size);
		UnorderedAccessBuffer(Device& dev, UINT64 size);
		void CreateUnorderedAccessView(Device& dev);
		inline TextureFormat Format() const { return format; };
	private:
		D3D12_CPU_DESCRIPTOR_HANDLE uav_cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE uav_gpu;
		TextureFormat format;

	private:
		friend ShaderTable;
		friend MasterNet;
		friend ConvLayer;
		friend PixelShuffle;
		friend AddLayer;
	};

}