#include "pipeline_state.h"
#include "internal/egx_internal.h"
#include "root_signature.h"
#include "input_layout.h"
#include "shader.h"
#include "device.h"

egx::PipelineState::PipelineState()
	: desc({})
{
	desc.SampleMask = UINT_MAX;
	desc.NumRenderTargets = 0;
	desc.SampleDesc.Count = 1;
}

void egx::PipelineState::SetRootSignature(RootSignature& root_signature)
{
	desc.pRootSignature = root_signature.root_signature.Get();
	assert(desc.pRootSignature != nullptr);
}
void egx::PipelineState::SetInputLayout(InputLayout& input_layout)
{
	desc.InputLayout.NumElements = (UINT)input_layout.element_descs.size();
	desc.InputLayout.pInputElementDescs = input_layout.element_descs.data();
}
void egx::PipelineState::SetPrimitiveTopology(TopologyType top)
{
	desc.PrimitiveTopologyType = convertTopologyType(top);
}
void egx::PipelineState::SetVertexShader(Shader& vertex_shader)
{
	desc.VS = CD3DX12_SHADER_BYTECODE(vertex_shader.shader_blob.Get());
}
void egx::PipelineState::SetPixelShader(Shader& pixel_shader)
{
	desc.PS = CD3DX12_SHADER_BYTECODE(pixel_shader.shader_blob.Get());
}
void egx::PipelineState::SetDepthStencilFormat(TextureFormat format)
{
	assert(isDepthFormat(format));
	desc.DSVFormat = convertToDepthStencilFormat(format);
}

void egx::PipelineState::SetRenderTargetFormat(TextureFormat format)
{
	desc.RTVFormats[0] = convertFormat(format);
	for (int i = 1; i < 8; i++)
		desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	if(format == TextureFormat::UNKNOWN)
		desc.NumRenderTargets = 0;
	else
		desc.NumRenderTargets = 1;
}
void egx::PipelineState::SetRenderTargetFormats(TextureFormat format1, TextureFormat format2)
{
	desc.RTVFormats[0] = convertFormat(format1);
	desc.RTVFormats[1] = convertFormat(format2);
	for (int i = 2; i < 8; i++)
		desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	desc.NumRenderTargets = 2;
}
void egx::PipelineState::SetRenderTargetFormats(TextureFormat format1, TextureFormat format2, TextureFormat format3)
{
	desc.RTVFormats[0] = convertFormat(format1);
	desc.RTVFormats[1] = convertFormat(format2);
	desc.RTVFormats[2] = convertFormat(format3);
	for (int i = 3; i < 8; i++)
		desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	desc.NumRenderTargets = 3;
}
void egx::PipelineState::SetRenderTargetFormats(TextureFormat format1, TextureFormat format2, TextureFormat format3, TextureFormat format4)
{
	desc.RTVFormats[0] = convertFormat(format1);
	desc.RTVFormats[1] = convertFormat(format2);
	desc.RTVFormats[2] = convertFormat(format3);
	desc.RTVFormats[3] = convertFormat(format4);
	for (int i = 4; i < 8; i++)
		desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	desc.NumRenderTargets = 4;
}

void egx::PipelineState::SetBlendState(const BlendState& blend_state)
{
	desc.BlendState = blend_state;
}
void egx::PipelineState::SetRasterState(const RasterState& raster_state)
{
	desc.RasterizerState = raster_state;
}
void egx::PipelineState::SetDepthStencilState(const DepthStencilState& depth_stencil_state)
{
	desc.DepthStencilState = depth_stencil_state;
}

void egx::PipelineState::Finalize(Device& dev)
{
	THROWIFFAILED(dev.device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pso)), "Failed to create pipeline state object");
}