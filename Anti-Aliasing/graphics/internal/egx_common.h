#pragma once
#include <d3d12.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

namespace egx
{
	// Forward declarations
	class CommandContext;
	class DepthBuffer;
	class Device;
	class CPUBuffer;
	class GPUBuffer;
	class Texture2D;
	class UploadHeap;
	class RootSignature;
	class Sampler;

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
	
	enum class TextureFormat
	{
		UNORM4x8
	};

	enum class DepthFormat
	{
		D16, D24_S8, D32, D32_S32
	};
}