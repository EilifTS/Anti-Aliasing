#pragma once
#include "internal/gpu_buffer.h"
#include "../math/point2d.h"

namespace egx
{
	enum class TextureFormat
	{
		UNORM4x8
	};

	class Texture2D : public GPUBuffer
	{
	public:
		Texture2D(Device& dev, TextureFormat format, const ema::point2D& size);

		void CreateShaderResourceView(Device& dev, TextureFormat format);
		inline void CreateShaderResourceView(Device& dev) { CreateShaderResourceView(dev, format); };
		void CreateRenderTargetView(Device& dev);
		const ema::point2D& Size() const { return size; };

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE srv;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
		TextureFormat format;
		ema::point2D size;
	};
}