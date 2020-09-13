#pragma once
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include <dxgi1_6.h>
#include <vector>
#include <memory>

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
		Device(const Window& window, const eio::InputManager& im, bool v_sync);

		void WaitForGPU();
		void PrepareNextFrame();

		void ScheduleUpload(CommandContext& context, const CPUBuffer& cpu_buffer, GPUBuffer& gpu_buffer);

		void QueueList(CommandContext& context);
		void Present(CommandContext& context);

	public:
		// Descriptor heaps
		static const int max_descriptors_in_heap = 64;
		std::unique_ptr<DescriptorHeap> buffer_heap;
		std::unique_ptr<DescriptorHeap> sampler_heap;
		std::unique_ptr<DescriptorHeap> rtv_heap;
		std::unique_ptr<DescriptorHeap> dsv_heap;

	private:
		static const int frame_count = 3;
		bool v_sync;
		bool use_warp;

		ComPtr<IDXGIAdapter4> adapter;			// Graphics card
		ComPtr<IDXGIOutput> adapter_output;		// Monitor
		ComPtr<ID3D12Device6> device;
		ComPtr<ID3D12CommandQueue> command_queue;
		ComPtr<IDXGISwapChain4> swap_chain;
		
		std::vector<RenderTarget> back_buffers;
		std::vector<ComPtr<ID3D12CommandAllocator>> command_allocators;
		
		// Upload
		std::vector<UploadHeap> upload_heaps[frame_count];

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
		friend CommandContext;
		friend RenderTarget;

	};
}