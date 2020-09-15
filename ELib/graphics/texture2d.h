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

		void CreateShaderResourceView(Device& dev, TextureFormat format);
		inline void CreateShaderResourceView(Device& dev) { CreateShaderResourceView(dev, format); };
		inline const ema::point2D& Size() const { return size; };
		inline TextureFormat Format() const { return format; };

	protected:
		
		Texture2D(ComPtr<ID3D12Resource> buffer, D3D12_RESOURCE_STATES start_state); // Constuctor used for back buffers and wic loading
		Texture2D(Device& dev,
			DXGI_FORMAT format,
			const ema::point2D& size,
			D3D12_RESOURCE_FLAGS flags,
			ClearValue clear_value); // Constructor used by rt and ds

		// Getters
		const D3D12_CPU_DESCRIPTOR_HANDLE& getSRVCPU() const;
		const D3D12_GPU_DESCRIPTOR_HANDLE& getSRVGPU() const;
		
	protected:
		D3D12_CPU_DESCRIPTOR_HANDLE srv_cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE srv_gpu;
		TextureFormat format;
		ema::point2D size;

	private:
		friend Device;
		friend CommandContext;
		friend std::shared_ptr<egx::Texture2D> eio::LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name);
	};

}