#pragma once
#include "../internal/egx_common.h"

namespace egx
{
	class ShaderLibrary
	{
	public:
		void Compile(const std::string& file_path);

	private:
		ComPtr<ID3DBlob> blob;

		friend RTPipelineState;
	};
}