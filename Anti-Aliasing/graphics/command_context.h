#pragma once
#include "internal/egx_common.h"
#include "../math/color.h"
#include <memory>

namespace egx
{
	class CommandContext
	{
	public:
		CommandContext(Device& dev);

		void ClearRenderTarget(Texture2D& target, const ema::color& color);
		void ClearDepth(DepthBuffer& buffer, float depth);
		void ClearStencil(DepthBuffer& buffer);
		void ClearDepthStencil(DepthBuffer& buffer, float depth);

		void SetPipelineState(PipelineState& pipeline_state);

		void SetRenderTarget(Texture2D& target);
		void SetRenderTarget(Texture2D& target, DepthBuffer& buffer);
		void SetDepthStencilBuffer(DepthBuffer& buffer);
		inline void SetBBAsRenderTarget() { SetRenderTarget(*current_bb); };
		inline void SetBBAsRenderTarget(DepthBuffer& buffer) { SetRenderTarget(*current_bb, buffer); };

		void SetTransitionBuffer(GPUBuffer& buffer, GPUBufferState new_state);

		void SetViewport(const ema::point2D& size);
		void SetScissor(const ema::point2D& size);
		void SetPrimitiveTopology(Topology top);

		// TODO: Add root signature values
		void SetRootSignature(RootSignature& root_signature);
		void SetRootConstant(int root_index, int num_constants, void* constant_data);
		void SetRootConstantBuffer(int root_index, ConstantBuffer& texture);
		void SetRootShaderResource(int root_index, Texture2D& texture);

		void SetVertexBuffer(const VertexBuffer& buffer);
		void SetIndexBuffer(const IndexBuffer& buffer);

		void Draw();
		void DrawIndexed();

	private:
		void copyFromUploadHeap(GPUBuffer& dest, UploadHeap& src);

	private:
		ComPtr<ID3D12GraphicsCommandList> command_list;
		Texture2D* current_bb;

	private:
		friend Device;
	};
}