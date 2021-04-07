#pragma once
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxcompiler.lib")

#include <dxgi1_6.h>
#include <vector>

#include "internal/descriptor_heap.h"
#include "../io/input_manager.h"
#include "internal/egx_common.h"
#include "internal/upload_heap.h"
#include "render_target.h"

class Window;

namespace egx
{
	class Device
	{
	public:
		Device();
		Device(const Window& window, const eio::InputManager& im, bool v_sync);
		~Device();

		bool SupportsRayTracing() const { return supports_rt; };

		void WaitForGPU();
		void PrepareNextFrame();

		void ScheduleUpload(CommandContext& context, const CPUBuffer& cpu_buffer, GPUBuffer& gpu_buffer);

		void QueueList(CommandContext& context);
		void QueueListAndWaitForFinish(CommandContext& context);
		void Present(CommandContext& context);

	public:
		// Descriptor heaps
		static const int max_descriptors_in_heap = 256;
		std::unique_ptr<DescriptorHeap> buffer_heap;
		std::unique_ptr<DescriptorHeap> sampler_heap;
		std::unique_ptr<DescriptorHeap> rtv_heap;
		std::unique_ptr<DescriptorHeap> dsv_heap;

	private:
		static const int frame_count = 3;
		bool v_sync;
		bool use_warp;
		bool supports_rt;

		ComPtr<IDXGIAdapter4> adapter;			// Graphics card
		ComPtr<IDXGIOutput> adapter_output;		// Monitor
		ComPtr<ID3D12Device5> device;
		ComPtr<ID3D12CommandQueue> command_queue;
		ComPtr<IDXGISwapChain4> swap_chain;
		
		std::vector<RenderTarget> back_buffers;
		std::vector<ComPtr<ID3D12CommandAllocator>> command_allocators;
		
		// Upload
		int first_frame_heap_chunk_size = 100 * (1024) * (1024); // 100MB
		int heap_chunk_size = 1 * (1024) * (1024); // 1MB
		std::vector<DynamicUploadHeap> upload_heaps;

		// Frame buffering
		int current_frame;
		UINT64 fence_values[frame_count];
		HANDLE fence_event;
		ComPtr<ID3D12Fence> fence;

	private:
		void initializeFence();
		void getBackBuffers();

	private:
		friend ConstantBuffer;
		friend UploadHeap;
		friend GPUBuffer;
		friend Texture2D;
		friend DepthBuffer;
		friend RootSignature;
		friend PipelineState;
		friend ComputePipelineState;
		friend CommandContext;
		friend RenderTarget;
		friend Mesh;
		friend TLAS;
		friend RTPipelineState;
		friend UnorderedAccessBuffer;
		friend MasterNet;
		friend std::shared_ptr<egx::Texture2D> eio::LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name, bool use_srgb);
		friend void eio::SaveTextureToFile(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, const std::string& file_name);
		friend void eio::SaveTextureToFileDDS(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, const std::string& file_name);
	};
}