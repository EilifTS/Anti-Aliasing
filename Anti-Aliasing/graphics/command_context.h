#pragma once
#include "internal/egx_common.h"
#include "../math/color.h"

namespace egx
{
	class CommandContext
	{
	public:
		CommandContext();

		void ClearRenderTarget(Texture2D& target, const ema::color& color);
		void ClearDepth(DepthBuffer& buffer, float depth);
		void ClearStencil(DepthBuffer& buffer);
		void ClearDepthStencil(DepthBuffer& buffer, float depth);

		void SetRootSignature();

		void SetRenderTarget(Texture2D& target);
		void SetRenderTarget(Texture2D& target, DepthBuffer& buffer);
		void SetDepthStencilBuffer(DepthBuffer& buffer);
		inline void SetBBAsRenderTarget() { SetRenderTarget(*current_bb); };
		inline void SetBBAsRenderTarget(DepthBuffer& buffer) { SetRenderTarget(*current_bb, buffer); };

		void TransitionBuffer(GPUBuffer& buffer, GPUBufferState new_state);

		void SetViewport();
		void SetScissor();
		void SetPrimitiveTopology();

		// TODO: Add root signature values

		void SetIndexBuffer();
		void SetVertexBuffer();

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