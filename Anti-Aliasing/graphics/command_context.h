#pragma once

namespace egx
{
	class Device;
	class CommandContext
	{
	public:
		CommandContext();

		void ClearRenderTarget();
		void ClearDepth();
		void ClearStencil();
		void ClearDepthStencil();

		void SetRootSignature();

		void SetRenderTarget();
		void SetDepthStencilTarget();

		void SetViewport();
		void SetScissor();
		void SetPrimitiveTopology();

		// TODO: Add root signature values

		void SetIndexBuffer();
		void SetVertexBuffer();

		void Draw();
		void DrawIndexed();
	private:
		ComPtr<ID3D12GraphicsCommandList> command_list;
	private:
		// Should be called at the start of each frame
		void reset();

	private:
		friend Device;
	};
}