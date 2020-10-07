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
#include "ray_tracing/shader_table.h"
#include "ray_tracing/rt_pipeline_state.h"

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

void egx::CommandContext::ClearRenderTarget(RenderTarget& target)
{
	command_list->ClearRenderTargetView(target.getRTV(), target.clear_value.Color, 0, nullptr);
}
void egx::CommandContext::ClearDepth(DepthBuffer& buffer)
{
	command_list->ClearDepthStencilView(buffer.getDSV(), D3D12_CLEAR_FLAG_DEPTH, buffer.clear_value.DepthStencil.Depth, 0, 0, nullptr);
}
void egx::CommandContext::ClearStencil(DepthBuffer& buffer)
{
	command_list->ClearDepthStencilView(buffer.getDSV(), D3D12_CLEAR_FLAG_STENCIL, 0.0f, buffer.clear_value.DepthStencil.Stencil, 0, nullptr);
}
void egx::CommandContext::ClearDepthStencil(DepthBuffer& buffer)
{
	command_list->ClearDepthStencilView(
		buffer.getDSV(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
		buffer.clear_value.DepthStencil.Depth, 
		buffer.clear_value.DepthStencil.Stencil, 
		0, nullptr);
}

void egx::CommandContext::SetPipelineState(PipelineState& pipeline_state)
{
	auto* pso = pipeline_state.pso.Get();
	assert(pso != nullptr);
	command_list->SetPipelineState(pso);
}
void egx::CommandContext::SetRTPipelineState(RTPipelineState& pipeline_state)
{
	auto* pso = pipeline_state.state_object.Get();
	assert(pso != nullptr);
	command_list->SetPipelineState1(pso);
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
void egx::CommandContext::SetRenderTargets(RenderTarget& target1, RenderTarget& target2, DepthBuffer& buffer)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = { target1.getRTV(), target2.getRTV() };
	D3D12_CPU_DESCRIPTOR_HANDLE dsvs[] = { buffer.getDSV() };
	command_list->OMSetRenderTargets(2, rtvs, FALSE, dsvs);
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
void egx::CommandContext::SetUABarrier(GPUBuffer& ua_buffer)
{
	D3D12_RESOURCE_BARRIER desc = {};
	desc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	desc.UAV.pResource = ua_buffer.buffer.Get();
	command_list->ResourceBarrier(1, &desc);
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
void egx::CommandContext::SetStencilRefrenceValue(unsigned char value)
{
	command_list->OMSetStencilRef((UINT)value);
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
void egx::CommandContext::SetComputeRootSignature(RootSignature& root_signature)
{
	auto* root_sig = root_signature.root_signature.Get();
	assert(root_sig != nullptr);
	command_list->SetComputeRootSignature(root_sig);
}
void egx::CommandContext::SetRootConstant(int root_index, int num_constants, void* constant_data)
{
	command_list->SetGraphicsRoot32BitConstants(root_index, num_constants, constant_data, 0);
}
void egx::CommandContext::SetRootConstantBuffer(int root_index, const ConstantBuffer& buffer)
{
	command_list->SetGraphicsRootConstantBufferView(root_index, buffer.buffer->GetGPUVirtualAddress());
}
void egx::CommandContext::SetRootDescriptorTable(int root_index, const Texture2D& first_texture)
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

void egx::CommandContext::CopyBuffer(const GPUBuffer& src, GPUBuffer& dest)
{
	command_list->CopyResource(dest.buffer.Get(), src.buffer.Get());
}

void egx::CommandContext::Draw(int vertex_count)
{
	command_list->DrawInstanced(vertex_count, 1, 0, 0);
}
void egx::CommandContext::DrawIndexed(int index_count)
{
	command_list->DrawIndexedInstanced(index_count, 1, 0, 0, 0);
}

void egx::CommandContext::DispatchRays(const ema::point2D& dims, ShaderTable& shader_table)
{
	D3D12_DISPATCH_RAYS_DESC dis_desc = {};
	dis_desc.Width = dims.x;
	dis_desc.Height = dims.y;
	dis_desc.Depth = 1;

	auto start_addess = shader_table.shader_table_heap->buffer->GetGPUVirtualAddress();
	dis_desc.RayGenerationShaderRecord.StartAddress = start_addess + shader_table.ray_gen_table_start;
	dis_desc.RayGenerationShaderRecord.SizeInBytes = shader_table.ray_gen_entry_size;
	
	dis_desc.MissShaderTable.StartAddress = start_addess + shader_table.miss_table_start;
	dis_desc.MissShaderTable.StrideInBytes = shader_table.miss_entry_size;
	dis_desc.MissShaderTable.SizeInBytes = shader_table.miss_table_size;

	dis_desc.HitGroupTable.StartAddress = start_addess + shader_table.hit_table_start;
	dis_desc.HitGroupTable.StrideInBytes = shader_table.hit_entry_size;
	dis_desc.HitGroupTable.SizeInBytes = shader_table.hit_table_size;

	command_list->DispatchRays(&dis_desc);
}

void egx::CommandContext::copyBufferFromUploadHeap(GPUBuffer& dest, UploadHeap& src, int heap_offset)
{
	command_list->CopyBufferRegion(dest.buffer.Get(), 0, src.buffer.Get(), heap_offset, dest.GetBufferSize());
}

void egx::CommandContext::copyTextureFromUploadHeap(GPUBuffer& dest, UploadHeap& src, int sub_res, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint)
{
	CD3DX12_TEXTURE_COPY_LOCATION dest_loc(dest.buffer.Get(), sub_res);
	CD3DX12_TEXTURE_COPY_LOCATION src_loc(src.buffer.Get(), footprint);
	command_list->CopyTextureRegion(&dest_loc, 0, 0, 0, &src_loc, nullptr);
}