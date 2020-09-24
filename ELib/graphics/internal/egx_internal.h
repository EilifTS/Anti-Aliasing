#pragma once
#include "../../io/console.h"
#include "d3dx12.h"
#include <wrl/client.h>
#include <stdexcept>
#include <assert.h>
#include "egx_common.h"

#define THROWIFFAILED(exp, error) if(FAILED(exp)) throw std::runtime_error(error)
using Microsoft::WRL::ComPtr;

namespace egx
{
	inline D3D12_RESOURCE_STATES convertResourceState(GPUBufferState state)
	{
		switch (state)
		{
		case egx::GPUBufferState::VertexBuffer:		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case egx::GPUBufferState::IndexBuffer:		return D3D12_RESOURCE_STATE_INDEX_BUFFER;
		case egx::GPUBufferState::ConstantBuffer:	return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		case egx::GPUBufferState::RenderTarget:		return D3D12_RESOURCE_STATE_RENDER_TARGET;
		case egx::GPUBufferState::DepthWrite:		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
		case egx::GPUBufferState::DepthRead:		return D3D12_RESOURCE_STATE_DEPTH_READ;
		case egx::GPUBufferState::NonPixelResource: return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		case egx::GPUBufferState::PixelResource:	return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		case egx::GPUBufferState::CopyDest:			return D3D12_RESOURCE_STATE_COPY_DEST;
		case egx::GPUBufferState::CopySource:		return D3D12_RESOURCE_STATE_COPY_SOURCE;
		case egx::GPUBufferState::Present:			return D3D12_RESOURCE_STATE_PRESENT;
		default: throw std::runtime_error("Invalid GPUBuffer state");
		}
		return D3D12_RESOURCE_STATE_COMMON;
	}

	inline D3D12_PRIMITIVE_TOPOLOGY convertTopology(Topology top)
	{
		switch (top)
		{
		case egx::Topology::LineList: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case egx::Topology::PointList: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case egx::Topology::TriangleList: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case egx::Topology::TriangleStrip: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default:
			throw std::runtime_error("Invalid primitive topology");
		}
	}
	inline D3D12_PRIMITIVE_TOPOLOGY_TYPE convertTopologyType(TopologyType top)
	{
		switch (top)
		{
		case egx::TopologyType::Line: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case egx::TopologyType::Patch: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		case egx::TopologyType::Point: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case egx::TopologyType::Triangle: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case egx::TopologyType::Undefined: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		default:
			throw std::runtime_error("Invalid primitive topology type");
		}
	}

	inline TextureFormat revertFormat(DXGI_FORMAT format)
	{
		return (TextureFormat)format;
	}

	inline DXGI_FORMAT convertFormat(egx::TextureFormat format)
	{
		return (DXGI_FORMAT)format;
	}
	inline bool isDepthFormat(egx::TextureFormat format)
	{
		switch (format)
		{
		case egx::TextureFormat::D16:
		case egx::TextureFormat::D24_S8:
		case egx::TextureFormat::D32:
		case egx::TextureFormat::D32_S32: 
			return true;
		}
		return false;
	}
	inline DXGI_FORMAT convertToDepthStencilFormat(egx::TextureFormat format)
	{
		switch (format)
		{
		case egx::TextureFormat::D16:     return DXGI_FORMAT_D16_UNORM;
		case egx::TextureFormat::D24_S8:  return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case egx::TextureFormat::D32:     return DXGI_FORMAT_D32_FLOAT;
		case egx::TextureFormat::D32_S32: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		}
		assert(0);
		return DXGI_FORMAT_UNKNOWN;
	}

	inline int formatByteSize(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8_UNORM:
			return 1;
		case DXGI_FORMAT_D16_UNORM:
			return 2;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return 4;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return 8;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return 16;
		default: throw std::runtime_error("Unsupported texture format");
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	inline D3D12_SHADER_VISIBILITY convertVisibility(egx::ShaderVisibility vis)
	{
		switch (vis)
		{
		case egx::ShaderVisibility::All: return D3D12_SHADER_VISIBILITY_ALL;
		case egx::ShaderVisibility::Vertex: return D3D12_SHADER_VISIBILITY_VERTEX;
		case egx::ShaderVisibility::Pixel: return D3D12_SHADER_VISIBILITY_PIXEL;
		default:
			throw std::runtime_error("Invalid shader visibility");
		}
	}
	
	// Align uLocation to the next multiple of uAlign.
	static UINT Align(UINT uLocation, UINT uAlign)
	{
		if ((0 == uAlign) || (uAlign & (uAlign - 1)))
		{
			throw std::runtime_error("non-pow2 alignment");
		}

		return ((uLocation + (uAlign - 1)) & ~(uAlign - 1));
	}
		
}