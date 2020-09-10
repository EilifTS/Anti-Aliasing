#pragma once
#include "internal/egx_common.h"
#include "texture2d.h"
#include "../math/point2d.h"

namespace egx
{
	class DepthBuffer : public Texture2D
	{
	public:
		DepthBuffer(Device& dev, DepthFormat format, const ema::point2D& size);

		void CreateDepthStencilView(Device& dev);
		const ema::point2D& Size() const { return size; };

	private:
		const D3D12_CPU_DESCRIPTOR_HANDLE& getDSV() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE dsv;

	private:
		friend CommandContext;
	};
}