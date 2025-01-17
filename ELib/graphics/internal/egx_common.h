#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <string>
using Microsoft::WRL::ComPtr;



namespace egx
{
	// Forward declarations
	class CommandContext;
	class ConstantBuffer;
	class DepthBuffer;
	class Device;
	class CPUBuffer;
	class GPUBuffer;
	class Texture2D;
	class UploadHeap;
	class RootSignature;
	class Sampler;
	class Shader;
	class PipelineState;
	class ComputePipelineState;
	class InputLayout;
	class BlendState;
	class VertexBuffer;
	class IndexBuffer;
	class RenderTarget;
	class DescriptorHeap;
	class Mesh;
	class Model;
	class TLAS;
	class RTPipelineState;
	class ShaderLibrary;
	class ShaderTable;
	class UnorderedAccessBuffer;
	class MasterNet;
	class ConvLayer;
	class PixelShuffle;
	class AddLayer;

	enum class GPUBufferState
	{
		VertexBuffer =			D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		IndexBuffer =			D3D12_RESOURCE_STATE_INDEX_BUFFER,
		ConstantBuffer =		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		RenderTarget =			D3D12_RESOURCE_STATE_RENDER_TARGET,
		DepthWrite =			D3D12_RESOURCE_STATE_DEPTH_WRITE,
		DepthRead =				D3D12_RESOURCE_STATE_DEPTH_READ,
		NonPixelResource =		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		PixelResource =			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		CopyDest =				D3D12_RESOURCE_STATE_COPY_DEST,
		CopySource =			D3D12_RESOURCE_STATE_COPY_SOURCE,
		Present =				D3D12_RESOURCE_STATE_PRESENT,
		UnorderedAccess =		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		AccelerationStructure = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE
	};
	inline GPUBufferState operator |(GPUBufferState lhs, GPUBufferState rhs) { return (GPUBufferState)((unsigned int)lhs | (unsigned int)rhs); }
	
	enum class Topology
	{
		LineList, PointList, TriangleList, TriangleStrip
	};
	enum class TopologyType
	{
		Line, Patch, Point, Triangle, Undefined
	};

	enum class TextureFormat
	{
		UNORM8x1 =			DXGI_FORMAT_R8_UNORM,
		UNORM8x4 =			DXGI_FORMAT_R8G8B8A8_UNORM, 
		UNORM8x4SRGB =		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		FLOAT16x2 =			DXGI_FORMAT_R16G16_FLOAT,
		FLOAT16x4 =			DXGI_FORMAT_R16G16B16A16_FLOAT,
		FLOAT32x1 =			DXGI_FORMAT_R32_FLOAT,
		FLOAT32x2 =			DXGI_FORMAT_R32G32_FLOAT,
		FLOAT32x4 =			DXGI_FORMAT_R32G32B32A32_FLOAT,

		D16 =				DXGI_FORMAT_R16_TYPELESS,
		D24_S8 =			DXGI_FORMAT_R24G8_TYPELESS,
		D32 =				DXGI_FORMAT_R32_TYPELESS,
		D32_S32 =			DXGI_FORMAT_R32G8X24_TYPELESS,

		D24_S8_Depth =		DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
		D24_S8_Stencil =	DXGI_FORMAT_X24_TYPELESS_G8_UINT,
		D32_Depth =			DXGI_FORMAT_R32_FLOAT,
		D32_S32_Depth =		DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,
		D32_S32_Stencil =	DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,

		UNKNOWN =			DXGI_FORMAT_UNKNOWN
	};

	enum class ShaderVisibility
	{
		All, Vertex, Pixel
	};

}

namespace eio
{
	extern std::shared_ptr<egx::Texture2D> LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name, bool use_srgb);
	extern void SaveTextureToFile(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, const std::string& file_name);
	extern void SaveTextureToFileDDS(egx::Device& dev, egx::CommandContext& context, egx::Texture2D& texture, const std::string& file_name);
}