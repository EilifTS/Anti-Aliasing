#pragma once
#include "internal/egx_common.h"
#include "texture2d.h"
#include "../math/point2d.h"

namespace egx
{
	class RenderTarget : public Texture2D
	{
	public:
		inline RenderTarget(Device& dev, TextureFormat format, const ema::point2D& size)
			: RenderTarget(dev, format, size, ClearValue::ColorBlue)
		{};
		RenderTarget(Device& dev, TextureFormat format, const ema::point2D& size, ClearValue clear_value);

		void CreateRenderTargetView(Device& dev);

	private:
		
		RenderTarget(ComPtr<ID3D12Resource> buffer); // Constuctor used for back buffers
		
		void createRenderTargetViewForBB(Device& dev); // Used for back buffers

		const D3D12_CPU_DESCRIPTOR_HANDLE& getRTV() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;

	private:
		friend CommandContext;
		friend Device;
	};
}