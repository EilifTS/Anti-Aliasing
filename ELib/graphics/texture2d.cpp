#include "texture2d.h"
#include "internal/egx_internal.h"
#include "device.h"

egx::Texture2D::Texture2D(Device& dev, TextureFormat format, const ema::point2D& size)
	: size(size), srv_cpu(), srv_gpu(), format(convertFormat(format)),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		convertFormat(format),
		size.x, size.y, 1,
		1,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_NONE,
		ClearValue::None
		)
{
	
}

void egx::Texture2D::CreateShaderResourceView(Device& dev, TextureFormat format)
{
	createShaderResourceView(dev, convertFormat(format));
}

void egx::Texture2D::createShaderResourceView(Device& dev, DXGI_FORMAT format)
{
	auto buffer_desc = buffer->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
	desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	desc.Format = format;
	desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MipLevels = buffer_desc.MipLevels;

	srv_cpu = dev.buffer_heap->GetNextHandle();

	dev.device->CreateShaderResourceView(buffer.Get(), &desc, srv_cpu);
	srv_gpu = dev.buffer_heap->GetGPUHandle(srv_cpu);
}

const D3D12_CPU_DESCRIPTOR_HANDLE& egx::Texture2D::getSRVCPU() const
{
	assert(srv_cpu.ptr != 0);
	return srv_cpu;
}
const D3D12_GPU_DESCRIPTOR_HANDLE& egx::Texture2D::getSRVGPU() const
{
	assert(srv_gpu.ptr != 0);
	return srv_gpu;
}

egx::Texture2D::Texture2D(ComPtr<ID3D12Resource> buffer, D3D12_RESOURCE_STATES start_state)
	: GPUBuffer(buffer, start_state), size(), srv_cpu(), srv_gpu(), format()
{
	auto desc = buffer->GetDesc();

	size = { (int)desc.Width, (int)desc.Height };
	format = desc.Format;
}

egx::Texture2D::Texture2D(Device& dev,
	DXGI_FORMAT format,
	const ema::point2D& size,
	D3D12_RESOURCE_FLAGS flags,
	ClearValue clear_value)
	: size(size), srv_cpu(), srv_gpu(), format(format),
	GPUBuffer(
		dev,
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		format,
		size.x, size.y, 1,
		1,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		flags,
		clear_value
		)
{

}