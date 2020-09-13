#include "command_context.h"
#include "internal/egx_internal.h"
#include "internal/gpu_buffer.h"
#include "texture2d.h"
#include "render_target.h"
#include "internal/upload_heap.h"
#include "depth_buffer.h"
#include "root_signature.h"
#include "constant_buffer.h"
#include "pipeline_state.h"
#include "vertex_buffer.h"
#include "index_buffer.h"
#include "device.h"
#include "internal/descriptor_heap.h"

egx::CommandContext::CommandContext(Device& dev, const ema::point2D& window_size)
	: current_bb(nullptr), window_size(window_size)
{
	THROWIFFAILED(
		dev.device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			dev.command_allocators[dev.current_frame].Get(),
			nullptr,
			IID_PPV_ARGS(&command_list)),
		"Failed to create command list");

	current_bb = &dev.back_buffers[dev.current_frame];

	eio::Console::Log("Created: Command context");
	//THROWIFFAILED(command_list->Close(), "Failed to close command list");

}

void egx::CommandContext::ClearRenderTarget(RenderTarget& target, const ema::color& color)
{
	command_list->ClearRenderTargetView(target.getRTV(), reinterpret_cast<const float*>(&color), 0, nullptr);
}
void egx::CommandContext::ClearDepth(DepthBuffer& buffer, float depth)
{
	command_list->ClearDepthStencilView(buffer.getDSV(), D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}
void egx::CommandContext::ClearStencil(DepthBuffer& buffer)
{
	command_list->ClearDepthStencilView(buffer.getDSV(), D3D12_CLEAR_FLAG_STENCIL, 0.0f, 0, 0, nullptr);
}
void egx::CommandContext::ClearDepthStencil(DepthBuffer& buffer, float depth)
{
	command_list->ClearDepthStencilView(buffer.getDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, 0, 0, nullptr);
}

void egx::CommandContext::SetPipelineState(PipelineState& pipeline_state)
{
	auto* pso = pipeline_state.pso.Get();
	assert(pso != nullptr);
	command_list->SetPipelineState(pso);
}

void egx::CommandContext::SetRenderTarget(RenderTarget& target)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { target.getRTV() };
	command_list->OMSetRenderTargets(1, rtvs, FALSE, nullptr);
}
void egx::CommandContext::SetRenderTarget(RenderTarget& target, DepthBuffer& buffer)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { target.getRTV() };
	D3D12_CPU_DESCRIPTOR_HANDLE dsvs[] = { buffer.getDSV() };
	command_list->OMSetRenderTargets(1, rtvs, FALSE, dsvs);
}
void egx::CommandContext::SetDepthStencilBuffer(DepthBuffer& buffer)
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsvs[] = { buffer.getDSV() };
	command_list->OMSetRenderTargets(0, nullptr, FALSE, dsvs);
}

void egx::CommandContext::SetTransitionBuffer(GPUBuffer& buffer, GPUBufferState new_state)
{
	auto new_state_x = convertResourceState(new_state);
	auto old_state_x = buffer.state;

	if (new_state_x != old_state_x)
	{
		D3D12_RESOURCE_BARRIER desc = {};
		desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		desc.Transition.pResource = buffer.buffer.Get();
		desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		desc.Transition.StateBefore = old_state_x;
		desc.Transition.StateAfter = new_state_x;

		command_list->ResourceBarrier(1, &desc);

		buffer.state = new_state_x;
	}
}

void egx::CommandContext::SetViewport(const ema::point2D& size)
{
	D3D12_VIEWPORT viewport;
	viewport.TopLeftX = 0.0f;
	viewport.TopLeftY = 0.0f;
	viewport.MinDepth = D3D12_MIN_DEPTH;
	viewport.MaxDepth = D3D12_MAX_DEPTH;
	viewport.Width = (float)size.x;
	viewport.Height = (float)size.y;
	command_list->RSSetViewports(1, &viewport);
}
void egx::CommandContext::SetScissor(const ema::point2D& size)
{
	D3D12_RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = (LONG)size.x;
	rect.bottom = (LONG)size.y;
	command_list->RSSetScissorRects(1, &rect);
}
void egx::CommandContext::SetPrimitiveTopology(Topology top)
{
	auto t = convertTopology(top);
	command_list->IASetPrimitiveTopology(t);
}

void egx::CommandContext::SetDescriptorHeap(DescriptorHeap& heap)
{
	ID3D12DescriptorHeap* heaps[] = { heap.descriptor_heap.Get() };
	command_list->SetDescriptorHeaps(1, heaps);
}

void egx::CommandContext::SetRootSignature(RootSignature& root_signature)
{
	auto* root_sig = root_signature.root_signature.Get();
	assert(root_sig != nullptr);
	command_list->SetGraphicsRootSignature(root_sig);
}
void egx::CommandContext::SetRootConstant(int root_index, int num_constants, void* constant_data)
{
	command_list->SetGraphicsRoot32BitConstants(root_index, num_constants, constant_data, 0);
}
void egx::CommandContext::SetRootConstantBuffer(int root_index, const ConstantBuffer& buffer)
{
	command_list->SetGraphicsRootConstantBufferView(root_index, buffer.buffer->GetGPUVirtualAddress());
}
void egx::CommandContext::SetRootDescriptorTable(int root_index, Texture2D& first_texture)
{
	command_list->SetGraphicsRootDescriptorTable(root_index, first_texture.getSRVGPU());
}

void egx::CommandContext::SetVertexBuffer(const VertexBuffer& buffer)
{
	D3D12_VERTEX_BUFFER_VIEW views[] = { buffer.view };
	command_list->IASetVertexBuffers(0, 1, views);
}
void egx::CommandContext::SetIndexBuffer(const IndexBuffer& buffer)
{
	D3D12_INDEX_BUFFER_VIEW views[] = { buffer.view };
	command_list->IASetIndexBuffer(views);
}

void egx::CommandContext::Draw(int vertex_count)
{
	command_list->DrawInstanced(vertex_count, 1, 0, 0);
}
void egx::CommandContext::DrawIndexed(int index_count)
{
	command_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}

void egx::CommandContext::copyFromUploadHeap(GPUBuffer& dest, UploadHeap& src)
{
	auto desc_dest = dest.buffer->GetDesc();

	if (desc_dest.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		command_list->CopyBufferRegion(dest.buffer.Get(), 0, src.buffer.Get(), 0, dest.GetBufferSize());
	}
	else // Texture
	{
		CD3DX12_TEXTURE_COPY_LOCATION dest_loc(dest.buffer.Get(), 0);
		CD3DX12_TEXTURE_COPY_LOCATION src_loc(src.buffer.Get(), 0);
		command_list->CopyTextureRegion(&dest_loc, 0, 0, 0, &src_loc, nullptr);
	}
}