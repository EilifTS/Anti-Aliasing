#pragma once
#include "internal/egx_common.h"
#include <string>

namespace egx
{
	class Shader
	{
	public:

		void CompileVertexShader(const std::string& path);
		void CompilePixelShader(const std::string& path);
	private:
		ComPtr<ID3DBlob> shader_blob;

		friend PipelineState;
	};
}