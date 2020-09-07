#pragma once
#include "internal/egx_common.h"

namespace egx
{
	class BlendState : public D3D12_BLEND_DESC
	{
	public:
		static inline BlendState NoBlend()
		{
			BlendState out;
			D3D12_RENDER_TARGET_BLEND_DESC rt_desc;


			out.AlphaToCoverageEnable = FALSE;
			out.IndependentBlendEnable = FALSE;

			rt_desc.BlendEnable = FALSE;
			rt_desc.LogicOpEnable = FALSE;
			rt_desc.SrcBlend = D3D12_BLEND_ONE;
			rt_desc.DestBlend = D3D12_BLEND_ZERO;
			rt_desc.BlendOp = D3D12_BLEND_OP_ADD;
			rt_desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			rt_desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			rt_desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			rt_desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			rt_desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
				out.RenderTarget[i] = rt_desc;

			return out;
		}
	};
	class RasterState : public D3D12_RASTERIZER_DESC
	{
	public:
		static inline RasterState Default()
		{
			RasterState out;
			out.FillMode = D3D12_FILL_MODE_SOLID;
			out.CullMode = D3D12_CULL_MODE_BACK;
			out.FrontCounterClockwise = FALSE;
			out.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			out.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			out.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			out.DepthClipEnable = TRUE;
			out.MultisampleEnable = FALSE;
			out.AntialiasedLineEnable = FALSE;
			out.ForcedSampleCount = 0;
			out.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
			return out;
		}
	};
	class DepthStencilState : public D3D12_DEPTH_STENCIL_DESC
	{
	public:
		static inline DepthStencilState DepthOn()
		{
			DepthStencilState out;
			out.DepthEnable = TRUE;
			out.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			out.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			out.StencilEnable = FALSE;
			out.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			out.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			out.FrontFace = defaultStencilOp;
			out.BackFace = defaultStencilOp;
			return out;
		}
		static inline DepthStencilState DepthOff()
		{
			DepthStencilState out;
			out.DepthEnable = FALSE;
			out.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			out.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			out.StencilEnable = FALSE;
			out.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			out.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
			{ D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			out.FrontFace = defaultStencilOp;
			out.BackFace = defaultStencilOp;
			return out;
		}
	};

	class PipelineState
	{
	public:
		PipelineState();

		void SetRootSignature(RootSignature& root_signature);
		void SetInputLayout(InputLayout& input_layout);
		void SetPrimitiveTopology(TopologyType top);
		void SetVertexShader(Shader& vertex_shader);
		void SetPixelShader(Shader& pixel_shader);
		void SetDepthStencilFormat(DepthFormat format);
		void SetRenderTargetFormat(TextureFormat format);

		void SetBlendState(const BlendState& blend_state);
		void SetRasterState(const RasterState& raster_state);
		void SetDepthStencilState(const DepthStencilState& depth_stencil_state);
		void Finalize(Device& dev);

	private:
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
		ComPtr<ID3D12PipelineState> pso;

		friend CommandContext;
	};
}