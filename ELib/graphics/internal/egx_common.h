#pragma once
#include <d3d12.h>
#include <wrl/client.h>
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
		LineList, PointList, TriangleList
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

}