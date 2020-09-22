#pragma once
#include "internal/egx_common.h"
#include "texture2d.h"
#include "../math/point2d.h"
#include "../math/color.h"

namespace egx
{
	class RenderTarget : public Texture2D
	{
	public:
		inline RenderTarget(Device& dev, TextureFormat format, const ema::point2D& size)
			: RenderTarget(dev, format, size, {0.0f, 0.0f, 0.0f, 0.0f})
		{};
		RenderTarget(Device& dev, TextureFormat format, const ema::point2D& size, const ema::color& clear_value);

		void CreateRenderTargetView(Device& dev);

	private:
		
		RenderTarget(ComPtr<ID3D12Resource> buffer); // Constuctor used for back buffers
		
		void createRenderTargetViewForBB(Device& dev); // Used for back buffers

		const D3D12_CPU_DESCRIPTOR_HANDLE& getRTV() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
		D3D12_CLEAR_VALUE clear_value;

	private:
		friend CommandContext;
		friend Device;
	};
}