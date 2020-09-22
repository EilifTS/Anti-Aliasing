#pragma once
#include "internal/egx_common.h"
#include "../math/color.h"
#include <memory>

namespace egx
{
	class CommandContext
	{
	public:
		CommandContext(Device& dev, const ema::point2D& window_size);

		void ClearRenderTarget(RenderTarget& target);
		void ClearDepth(DepthBuffer& buffer);
		void ClearStencil(DepthBuffer& buffer);
		void ClearDepthStencil(DepthBuffer& buffer);

		void SetPipelineState(PipelineState& pipeline_state);

		void SetRenderTarget(RenderTarget& target);
		void SetRenderTarget(RenderTarget& target, DepthBuffer& buffer);
		void SetRenderTargets(RenderTarget& target1, RenderTarget& target2, DepthBuffer& buffer);
		void SetDepthStencilBuffer(DepthBuffer& buffer);

		void SetTransitionBuffer(GPUBuffer& buffer, GPUBufferState new_state);

		void SetViewport(const ema::point2D& size);
		inline void SetViewport(); // Set viewport to the whole screen
		void SetScissor(const ema::point2D& size);
		inline void SetScissor(); // Set scissor to the whole screen
		void SetPrimitiveTopology(Topology top);

		void SetDescriptorHeap(DescriptorHeap& heap);

		void SetRootSignature(RootSignature& root_signature);
		void SetRootConstant(int root_index, int num_constants, void* constant_data);
		void SetRootConstantBuffer(int root_index, const ConstantBuffer& texture);
		void SetRootDescriptorTable(int root_index, const Texture2D& first_texture);

		void SetVertexBuffer(const VertexBuffer& buffer);
		void SetIndexBuffer(const IndexBuffer& buffer);

		void CopyBuffer(const GPUBuffer& src, GPUBuffer& dest);

		void Draw(int vertex_count);
		void DrawIndexed(int index_count);

		inline RenderTarget& GetCurrentBackBuffer() { return *current_bb; };

	private:
		void copyBufferFromUploadHeap(GPUBuffer& dest, UploadHeap& src);
		void copyTextureFromUploadHeap(GPUBuffer& dest, UploadHeap& src, int sub_res, const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint);

	private:
		ComPtr<ID3D12GraphicsCommandList> command_list;
		RenderTarget* current_bb;
		ema::point2D window_size;

	private:
		friend Device;
	};
}

inline void egx::CommandContext::SetViewport()
{
	SetViewport(window_size);
}

inline void egx::CommandContext::SetScissor()
{
	SetScissor(window_size);
}