#pragma once
#include "internal/egx_common.h"
#include <vector>
#include <string>
#include <assert.h>

namespace egx
{
	class InputLayout
	{
	public:
		InputLayout()
			: element_descs(), pos_c(0), col_c(0), nor_c(0), tc_c(0)
		{

		}

		inline void AddPosition(int elements) 
		{
			addElement("POSITION", pos_c, elements);
			pos_c++;
		}
		inline void AddColor(int elements)
		{
			addElement("COLOR", col_c, elements);
			col_c++;
		}
		inline void AddNormal(int elements)
		{
			addElement("NORMAL", nor_c, elements);
			nor_c++;
		}
		inline void AddTextureCoordinate(int elements)
		{
			addElement("TEXCOORD", tc_c, elements);
			tc_c++;
		}
		inline void AddTextureCoordinateInt(int elements)
		{
			addElement("TEXCOORD", tc_c, elements, true);
			tc_c++;
		}

	private:
		std::vector<D3D12_INPUT_ELEMENT_DESC> element_descs;
		int pos_c, col_c, nor_c, tc_c;

	private:
		inline void addElement(const char* name, int nr, int elements, bool is_int = false)
		{
			auto format = elementsToFormat(elements);
			if (is_int)
				format = elementsToFormatInt(elements);

			element_descs.push_back({
				name, (UINT)nr, format, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
				});
		}

		inline DXGI_FORMAT elementsToFormat(int elements)
		{
			assert(elements > 0 && elements <= 4);
			switch (elements)
			{
			case 1: return DXGI_FORMAT_R32_FLOAT;
			case 2: return DXGI_FORMAT_R32G32_FLOAT;
			case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
			case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			return DXGI_FORMAT_UNKNOWN;
		}
		inline DXGI_FORMAT elementsToFormatInt(int elements)
		{
			assert(elements > 0 && elements <= 4);
			switch (elements)
			{
			case 1: return DXGI_FORMAT_R32_UINT;
			case 2: return DXGI_FORMAT_R32G32_UINT;
			case 3: return DXGI_FORMAT_R32G32B32_UINT;
			case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
			}
			return DXGI_FORMAT_UNKNOWN;
		}

		friend PipelineState;
	};
}