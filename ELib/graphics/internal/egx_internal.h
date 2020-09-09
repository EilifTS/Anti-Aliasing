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

	inline DXGI_FORMAT convertFormat(egx::TextureFormat format)
	{
		switch (format)
		{
		case egx::TextureFormat::UNORM4x8: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case egx::TextureFormat::UNORM4x8SRGB: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		default: throw std::runtime_error("Unsupported texture format");
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	inline DXGI_FORMAT convertDepthFormat(egx::DepthFormat format)
	{
		switch (format)
		{
		case egx::DepthFormat::D16:			return DXGI_FORMAT_D16_UNORM;
		case egx::DepthFormat::D24_S8:		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case egx::DepthFormat::D32:			return DXGI_FORMAT_D32_FLOAT;
		case egx::DepthFormat::D32_S32:		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		default: throw std::runtime_error("Unsupported depth format");
		}
		return DXGI_FORMAT_UNKNOWN;
	}

	inline int formatByteSize(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return 4;
		default: throw std::runtime_error("Unsupported texture format");
		}
		return 0;
	}
	inline int depthFormatByteSize(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_D16_UNORM:
			return 2;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_D32_FLOAT:
			return 4;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return 8;
		default: throw std::runtime_error("Unsupported depth format");
		}
		return 0;
	}
}