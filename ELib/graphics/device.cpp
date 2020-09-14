#include "device.h"
#include "internal/egx_internal.h"
#include "cpu_buffer.h"
#include "internal/gpu_buffer.h"
#include "internal/upload_heap.h"
#include "texture2d.h"
#include "../window/window.h"
#include "../misc/string_helpers.h"
#include "command_context.h"

#include <vector>
#include <assert.h>

namespace
{
	void enableDebugLayer()
	{
#if defined(_DEBUG)
		// Enable the D3D12 debug layer.
		{
			ComPtr<ID3D12Debug> debug_controller;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
			{
				debug_controller->EnableDebugLayer();
			}
		}
#endif
	}
	ComPtr<IDXGIFactory7> createFactory()
	{
		// Enumerate graphics adapters
		ComPtr<IDXGIFactory2> factory;
		ComPtr<IDXGIFactory7> factory7;
		UINT createFactoryFlags = 0;

#if defined(_DEBUG)
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		THROWIFFAILED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)), "Failed to create DXGIFactory");
		factory.As(&factory7);
		return factory7;
	}
	ComPtr<IDXGIAdapter4> getAdapter(ComPtr<IDXGIFactory7> factory)
	{
		// Find all adapters
		std::vector < ComPtr<IDXGIAdapter4> > adapters;
		ComPtr<IDXGIAdapter1> adapter1;
		ComPtr<IDXGIAdapter4> adapter4;
		for (unsigned int i = 0; factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			adapter1.As(&adapter4);
			adapters.push_back(adapter4);
		}

		eio::Console::Log("Number of adapters: " + emisc::ToString((int)adapters.size()));
		if (adapters.size() == 0)
			throw std::runtime_error("No adapters found");

		char video_card_description[128];
		int video_card_memory;
		for (auto adapter : adapters)
		{
			DXGI_ADAPTER_DESC adapter_desc;
			THROWIFFAILED(adapter->GetDesc(&adapter_desc), "Failed to get adapter description");

			video_card_memory = (int)(adapter_desc.DedicatedVideoMemory / 1024 / 1024); // memory in megabytes

			// Convert the name of the video card to a character array and store it
			unsigned long long description_length;
			int error = wcstombs_s(&description_length, video_card_description, 128, adapter_desc.Description, 128);
			if (error != 0)
				throw std::runtime_error("Failed to convert adapter description from wchar to char");

			eio::Console::Log("Adapter: " + std::string(video_card_description) + " VRAM size: " + emisc::ToString(video_card_memory) + "MB");
		}

		return adapters[0];

	}
	ComPtr<IDXGIOutput> getAdapterOutput(ComPtr<IDXGIAdapter4> adapter)
	{
		// Get displaymodes of adapter 0
		ComPtr<IDXGIOutput> adapter_output;

		THROWIFFAILED(adapter->EnumOutputs(0, &adapter_output), "Failed to find any outputs for the current adapter");

		DXGI_OUTPUT_DESC desc;
		adapter_output->GetDesc(&desc);

		char monitor_name[128];
		unsigned long long name_length;
		int error = wcstombs_s(&name_length, monitor_name, 128, desc.DeviceName, 128);
		if (error != 0)
			throw std::runtime_error("Failed to convert adapter description from wchar to char");

		eio::Console::Log("Monitor: " + std::string(monitor_name));

		return adapter_output;
	}
	ema::point2D getAdapterRefreshRate(ComPtr<IDXGIOutput> adapter_output, const ema::point2D& window_size)
	{
		// Get screen refreshrate used for v_sync
		unsigned int refreshrate_numerator = 0;
		unsigned int refreshrate_denominator = 0;

		unsigned int displaymode_num = 0;
		unsigned int displaymode_flags = DXGI_ENUM_MODES_INTERLACED;
		DXGI_FORMAT displaymode_format = DXGI_FORMAT_R8G8B8A8_UNORM;

		adapter_output->GetDisplayModeList(displaymode_format, displaymode_flags, &displaymode_num, 0);
		if (displaymode_num == 0)
			throw std::runtime_error("Could not find any display modes for the current format");
		eio::Console::Log("Display modes found: " + emisc::ToString(displaymode_num));

		std::vector<DXGI_MODE_DESC> displaymode_descs(displaymode_num);
		adapter_output->GetDisplayModeList(displaymode_format, displaymode_flags, &displaymode_num, &(displaymode_descs[0]));

		for (const auto& desc : displaymode_descs)
		{
			if (desc.Width == (unsigned int)window_size.x && desc.Height == (unsigned int)window_size.y)
			{
				refreshrate_numerator = desc.RefreshRate.Numerator;
				refreshrate_denominator = desc.RefreshRate.Denominator;
			}
		}
		eio::Console::Log("Refreshrate found: " + emisc::ToString(refreshrate_numerator) + " / " + emisc::ToString(refreshrate_denominator) + "Hz");

		return { (int)refreshrate_numerator, (int)refreshrate_denominator };
	}
	ComPtr<ID3D12Device6> createDevice(ComPtr<IDXGIAdapter4> adapter)
	{
		ComPtr<ID3D12Device> d3d12Device;
		ComPtr<ID3D12Device6> d3d12Device6;
		THROWIFFAILED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device)), "Could not create dx12 device");
		d3d12Device.As(&d3d12Device6);


		// Enable debug messages in debug mode.
#if defined(_DEBUG)
		ComPtr<ID3D12InfoQueue> pInfoQueue;
		if (SUCCEEDED(d3d12Device6.As(&pInfoQueue)))
		{
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		}
#endif
		eio::Console::Log("Created: Device");
		return d3d12Device6;
	}
	bool checkTearingSupport(ComPtr<IDXGIFactory7> factory)
	{
		BOOL allowTearing = FALSE;

		if (FAILED(factory->CheckFeatureSupport(
			DXGI_FEATURE_PRESENT_ALLOW_TEARING,
			&allowTearing, sizeof(allowTearing))))
		{
			allowTearing = FALSE;
		}
		return allowTearing == TRUE;
	}
	ComPtr<ID3D12CommandQueue> createCommandQueue(ComPtr<ID3D12Device5> device)
	{
		ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

		D3D12_COMMAND_QUEUE_DESC desc = {};
		desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		desc.NodeMask = 0;

		THROWIFFAILED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)), "Failed to create command queue");

		eio::Console::Log("Created: Command queue");
		return d3d12CommandQueue;
	}
	ComPtr<IDXGISwapChain4> createSwapChain(HWND hwnd, ComPtr<IDXGIFactory7> factory, ComPtr<ID3D12CommandQueue> command_queue, const ema::point2D& window_size, int buffer_count, bool allow_tearing)
	{
		ComPtr<IDXGISwapChain4> swap_chain;
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.Width = window_size.x;
		desc.Height = window_size.y;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Stereo = false;
		desc.SampleDesc = { 1, 0 };
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = buffer_count;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		desc.Flags = allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		ComPtr<IDXGISwapChain1> swap_chain1;
		THROWIFFAILED(factory->CreateSwapChainForHwnd(
			command_queue.Get(),
			hwnd, &desc,
			nullptr,
			nullptr,
			&swap_chain1
		), "Failed to create swap chain");

		THROWIFFAILED(swap_chain1.As(&swap_chain), "Failed to convert swapchain1 to swapchain4");

		eio::Console::Log("Created: Swap chain");

		return swap_chain;
	}
	std::vector< ComPtr<ID3D12CommandAllocator> > createCommandAllocators(ComPtr<ID3D12Device5> device, int frame_count)
	{
		std::vector<ComPtr<ID3D12CommandAllocator>> command_allocators(frame_count);
		for (int i = 0; i < frame_count; i++)
		{
			ComPtr<ID3D12CommandAllocator> command_allocator;
			THROWIFFAILED(
				device->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(&command_allocator)),
				"Failed to create command allocator"
			);
			command_allocators[i] = command_allocator;
		}
		

		eio::Console::Log("Created: Command allocators");

		return command_allocators;
	}
}

egx::Device::Device(const Window& window, const eio::InputManager& im, bool v_sync)
	: v_sync(v_sync), current_frame(0), fence_values(), fence_event()
{
	eio::Console::SetColor(3);
	enableDebugLayer();
	auto factory = createFactory();
	adapter = getAdapter(factory);
	adapter_output = getAdapterOutput(adapter);
	auto refreshrate = getAdapterRefreshRate(adapter_output, im.Window().WindowSize());
	device = createDevice(adapter);
	command_queue = createCommandQueue(device);
	bool allow_tearing = checkTearingSupport(factory);
	swap_chain = createSwapChain(window.Handle(), factory, command_queue, im.Window().WindowSize(), frame_count, allow_tearing);

	buffer_heap = std::make_unique<DescriptorHeap>(device, DescriptorType::Buffer, max_descriptors_in_heap);
	sampler_heap = std::make_unique<DescriptorHeap>(device, DescriptorType::Sampler, max_descriptors_in_heap);
	rtv_heap = std::make_unique<DescriptorHeap>(device, DescriptorType::RenderTarget, max_descriptors_in_heap);
	dsv_heap = std::make_unique<DescriptorHeap>(device, DescriptorType::DepthStencil, max_descriptors_in_heap);

	getBackBuffers();
	command_allocators = createCommandAllocators(device, frame_count);

	initializeFence();

	eio::Console::SetColor(15);
}

void egx::Device::WaitForGPU()
{
	// Scedule a Signal command in the queue
	THROWIFFAILED(command_queue->Signal(fence.Get(), fence_values[current_frame]), "Failed to create signal command");

	// Wait for gpu to process fence
	THROWIFFAILED(fence->SetEventOnCompletion(fence_values[current_frame], fence_event), "Failed to set fence event");
	WaitForSingleObjectEx(fence_event, INFINITE, FALSE);

	// Fence value is used up
	fence_values[current_frame]++;
}

void egx::Device::PrepareNextFrame()
{
	const auto fence_value = fence_values[current_frame];

	// Scedule a Signal command in the queue
	THROWIFFAILED(command_queue->Signal(fence.Get(), fence_value), "Failed to create signal command");

	// Update frame index
	current_frame = swap_chain->GetCurrentBackBufferIndex();

	// Wait untill new frame is ready
	if (fence->GetCompletedValue() < fence_values[current_frame])
	{
		THROWIFFAILED(fence->SetEventOnCompletion(fence_values[current_frame], fence_event), "Failed to set fence event");
		WaitForSingleObjectEx(fence_event, INFINITE, FALSE);
	}

	// Update fence value
	fence_values[current_frame] = fence_value + 1;
}

void egx::Device::ScheduleUpload(CommandContext& context, const CPUBuffer& cpu_buffer, GPUBuffer& gpu_buffer)
{
	//assert(cpu_buffer.Size() <= gpu_buffer.GetBufferSize());

	auto dest_desc = gpu_buffer.buffer->GetDesc();

	// Textures
	if (dest_desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		UINT64 upload_heap_size = 0;

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(dest_desc.MipLevels);

		device->GetCopyableFootprints(&dest_desc, 0, dest_desc.MipLevels, 0, footprints.data(), nullptr, nullptr, &upload_heap_size);

		
		int element_size = gpu_buffer.GetElementSize();

		// Create an upload heap for the intermediate step
		upload_heaps[current_frame].push_back(UploadHeap(*this, (int)upload_heap_size));
		auto& upload_heap = upload_heaps[current_frame].back();

		char* cpu_buffer_ptr = (char*)cpu_buffer.GetPtr();
		char* upload_heap_ptr = (char*)upload_heap.Map();

		// Copy over every row of the texture
		UINT64 upload_heap_offset = 0;
		UINT64 cpu_buffer_offset = 0;
		for (auto& footprint : footprints)
		{
			upload_heap_offset = footprint.Offset;
			for (int y = 0; y < (int)footprint.Footprint.Height; y++)
			{
				memcpy(
					upload_heap_ptr + upload_heap_offset + (UINT64)y * footprint.Footprint.RowPitch,
					cpu_buffer_ptr  + cpu_buffer_offset  + (UINT64)y * element_size * footprint.Footprint.Width,
					(long long)element_size * footprint.Footprint.Width);
			}
			cpu_buffer_offset += footprint.Footprint.Width * footprint.Footprint.Height * element_size;
		}
		

		upload_heap.Unmap();

		for(int i = 0; i < (int)footprints.size(); i++)
			context.copyTextureFromUploadHeap(gpu_buffer, upload_heap, i, footprints[i]);
	}
	else // Buffers
	{
		// Create an upload heap for the intermediate step
		upload_heaps[current_frame].push_back(UploadHeap(*this, gpu_buffer.GetBufferSize()));
		auto& upload_heap = upload_heaps[current_frame].back();

		void* upload_heap_ptr = upload_heap.Map();
		memcpy(upload_heap_ptr, cpu_buffer.GetPtr(), (size_t)cpu_buffer.Size());
		upload_heap.Unmap();

		context.copyBufferFromUploadHeap(gpu_buffer, upload_heap);
	}

}

void egx::Device::QueueList(CommandContext& context)
{
	auto* command_list = context.command_list.Get();
	THROWIFFAILED(command_list->Close(), "Failed to close command list");

	ID3D12CommandList* command_lists[] = { command_list };
	command_queue->ExecuteCommandLists(1, command_lists);

	command_list->Reset(command_allocators[current_frame].Get(), nullptr);
}
void egx::Device::Present(CommandContext& context)
{
	context.SetTransitionBuffer(back_buffers[current_frame], GPUBufferState::Present);
	QueueList(context);
	THROWIFFAILED(swap_chain->Present(0, 0), "Failed to present frame");
	PrepareNextFrame();
	THROWIFFAILED(command_allocators[current_frame]->Reset(), "Failed to reset command allocator");
	context.command_list->Close();
	context.command_list->Reset(command_allocators[current_frame].Get(), nullptr);
	context.current_bb = &(back_buffers[current_frame]);
	upload_heaps[current_frame].clear();
	
}

void egx::Device::initializeFence()
{
	current_frame = swap_chain->GetCurrentBackBufferIndex();
	fence_values[current_frame] = 0;
	THROWIFFAILED(device->CreateFence(fence_values[current_frame], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
		"Failed to create fence");
	fence_values[current_frame]++;
	
	fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fence_event == nullptr)
		THROWIFFAILED(HRESULT_FROM_WIN32(GetLastError()), "Failed to create fence event");

	eio::Console::Log("Created: fence");
}

void egx::Device::getBackBuffers()
{
	back_buffers.reserve(frame_count);

	for (int i = 0; i < frame_count; ++i)
	{
		ComPtr<ID3D12Resource> back_buffer;
		THROWIFFAILED(swap_chain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)), "Failed to get backbuffer");
		//auto rtv = rtv_heap.GetNextHandle();

		//device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtv);

		back_buffers.push_back(RenderTarget(back_buffer));
		back_buffers.back().createRenderTargetViewForBB(*this);
	}

	eio::Console::Log("Created: Back buffer views");
}




