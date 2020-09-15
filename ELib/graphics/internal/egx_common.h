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
	class InputLayout;
	class BlendState;
	class VertexBuffer;
	class IndexBuffer;
	class RenderTarget;
	class DescriptorHeap;

	enum class GPUBufferState
	{
		VertexBuffer,
		IndexBuffer,
		ConstantBuffer,
		RenderTarget,
		DepthWrite,
		DepthRead,
		NonPixelResource,
		PixelResource,
		CopyDest,
		CopySource,
		Present
	}; 
	
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
		UNORM8x4 =			DXGI_FORMAT_R8G8B8A8_UNORM, 
		UNORM8x4SRGB =		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		FLOAT16x4 =			DXGI_FORMAT_R16G16B16A16_FLOAT,
		FLOAT32x4 =			DXGI_FORMAT_R32G32B32A32_FLOAT,

		D16 =				DXGI_FORMAT_D16_UNORM,
		D24_S8 =			DXGI_FORMAT_D24_UNORM_S8_UINT,
		D32 =				DXGI_FORMAT_D32_FLOAT,
		D32_S32 =			DXGI_FORMAT_D32_FLOAT_S8X24_UINT
	};

	enum class ClearValue
	{
		Depth0, Depth1, ColorBlack, ColorBlue, Clear0001, None
	};

	enum class ShaderVisibility
	{
		All, Vertex, Pixel
	};

}

namespace eio
{
	extern std::shared_ptr<egx::Texture2D> LoadTextureFromFile(egx::Device& dev, egx::CommandContext& context, const std::string& file_name);
}