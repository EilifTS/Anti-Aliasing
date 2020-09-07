#pragma once
#include "internal/egx_common.h"
#include "internal/gpu_buffer.h"
#include "../math/point2d.h"

namespace egx
{
	

	class Texture2D : public GPUBuffer
	{
	public:
		Texture2D(Device& dev, TextureFormat format, const ema::point2D& size);

		inline void CreateShaderResourceView(Device& dev, TextureFormat format);
		inline void CreateShaderResourceView(Device& dev) { createShaderResourceView(dev, format); };
		void CreateRenderTargetView(Device& dev);
		const ema::point2D& Size() const { return size; };

	private:
		// Constuctor used for back buffers
		Texture2D(ComPtr<ID3D12Resource> buffer);
		void createShaderResourceView(Device& dev, DXGI_FORMAT format);
		// Used for back buffers
		void createRenderTargetViewForBB(Device& dev);
		
		// Getters
		const D3D12_CPU_DESCRIPTOR_HANDLE& getSRV() const;
		const D3D12_CPU_DESCRIPTOR_HANDLE& getRTV() const;
		
	private:
		D3D12_CPU_DESCRIPTOR_HANDLE srv;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
		DXGI_FORMAT format;
		ema::point2D size;

	private:
		friend Device;
		friend CommandContext;
	};
}