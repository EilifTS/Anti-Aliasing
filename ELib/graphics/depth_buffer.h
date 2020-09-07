#pragma once
#include "internal/egx_common.h"
#include "internal/gpu_buffer.h"
#include "../math/point2d.h"

namespace egx
{
	class DepthBuffer : public GPUBuffer
	{
	public:
		DepthBuffer(Device& dev, DepthFormat format, const ema::point2D& size);

		void CreateShaderResourceView(Device& dev);
		void CreateDepthStencilView(Device& dev);
		const ema::point2D& Size() const { return size; };

	private:
		const D3D12_CPU_DESCRIPTOR_HANDLE& getSRV() const;
		const D3D12_CPU_DESCRIPTOR_HANDLE& getDSV() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE srv;
		D3D12_CPU_DESCRIPTOR_HANDLE dsv;
		DXGI_FORMAT format;
		ema::point2D size;

	private:
		friend CommandContext;
	};
}