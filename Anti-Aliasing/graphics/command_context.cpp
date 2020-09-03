#include "command_context.h"
#include "internal/egx_internal.h"
#include "internal/gpu_buffer.h"
#include "texture2d.h"
#include "internal/upload_heap.h"
#include "depth_buffer.h"

void egx::CommandContext::ClearRenderTarget(Texture2D& target, const ema::color& color)
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

void egx::CommandContext::SetRenderTarget(Texture2D& target)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { target.getRTV() };
	command_list->OMSetRenderTargets(1, rtvs, FALSE, nullptr);
}
void egx::CommandContext::SetRenderTarget(Texture2D& target, DepthBuffer& buffer)
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

void egx::CommandContext::TransitionBuffer(GPUBuffer& buffer, GPUBufferState new_state)
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