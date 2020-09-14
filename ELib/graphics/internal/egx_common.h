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
		UNORM4x8, UNORM4x8SRGB
	};

	enum class DepthFormat
	{
		D16, D24_S8, D32, D32_S32
	};

	enum class ClearValue
	{
		Depth0, Depth1, ColorBlack, ColorBlue, None
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