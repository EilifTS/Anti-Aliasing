#pragma once
#include "internal/egx_common.h"
#include "texture2d.h"
#include "../math/point2d.h"

namespace egx
{
	class DepthBuffer : public Texture2D
	{
	public:
		inline DepthBuffer(Device& dev, TextureFormat format, const ema::point2D& size) : DepthBuffer(dev, format, size, 1.0f, 0) {};
		DepthBuffer(Device& dev, TextureFormat format, const ema::point2D& size, float depth_clear, unsigned char stencil_clear);

		void CreateDepthStencilView(Device& dev);
		const ema::point2D& Size() const { return size; };

	private:
		const D3D12_CPU_DESCRIPTOR_HANDLE& getDSV() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE dsv;
		D3D12_CLEAR_VALUE clear_value;

	private:
		friend CommandContext;
	};
}