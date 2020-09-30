#pragma once
#include "../internal/egx_common.h"
#include "shader_library.h"
#include <vector>

namespace egx
{
	class RTPipelineState
	{
	public:
		RTPipelineState()
		{
			libraries.reserve(1);
			hit_groups.reserve(1);
			rs_associations.reserve(3);
		}

		void AddLibrary(ShaderLibrary& lib, const std::vector<std::wstring>& exports);
		void AddHitGroup(const std::wstring& hit_group_name, const std::wstring& closest_hit_symbol);
		void AddHitGroup(const std::wstring& hit_group_name, const std::wstring& closest_hit_symbol, const std::wstring& any_hit_symbol, const std::wstring& intersection_symbol);
		void AddRootSignatureAssociation(RootSignature& rs, const std::vector<std::wstring>& symbols);
		void SetMaxPayloadSize(int new_value) { max_payload_size = new_value; };
		void SetMaxAttributeSize(int new_value) { max_attribute_size = new_value; };
		void SetMaxRecursionDepth(int new_value) { max_recursion_depth = new_value; };

		void Finalize(Device& dev);

	private:
		void buildShaderExportList(std::vector<std::wstring>& exported_symbols);

	private:
		struct library
		{
			library(ShaderLibrary& lib, const std::vector<std::wstring>& exports)
				: library(lib.blob, exports) {};
			library(const library& lib)
				: library(lib.byte_code, lib.export_symbols) {};

			ComPtr<ID3DBlob> byte_code;
			D3D12_DXIL_LIBRARY_DESC lib_desc;
			std::vector<D3D12_EXPORT_DESC> export_descs;
			std::vector<std::wstring> export_symbols;

		private:
			library(ComPtr<ID3DBlob> lib_blob, const std::vector<std::wstring>& exports);
		};

		struct hitGroup
		{
			hitGroup(const std::wstring& hit_group_name,
					 const std::wstring& closest_hit_symbol,
					 const std::wstring& any_hit_symbol,
					 const std::wstring& intersection_symbol);

			hitGroup(const hitGroup& hg)
				: hitGroup(hg.hit_group_name, hg.closest_hit_symbol, hg.any_hit_symbol, hg.intersection_symbol) {};

			std::wstring hit_group_name;
			std::wstring closest_hit_symbol;
			std::wstring any_hit_symbol;
			std::wstring intersection_symbol;
			D3D12_HIT_GROUP_DESC desc = {};

		};

		struct rsAssociation
		{
			rsAssociation(RootSignature& rs, const std::vector<std::wstring>& symbols);
			rsAssociation(const rsAssociation& rsa);

			ComPtr<ID3D12RootSignature> rs;
			std::vector<std::wstring> symbols;
			std::vector<LPCWSTR> symbol_pointers;
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association;
		};

		std::vector<library> libraries = {};
		std::vector<hitGroup> hit_groups = {};
		std::vector<rsAssociation> rs_associations = {};
		int max_payload_size = 0;
		int max_attribute_size = 0;
		int max_recursion_depth = 0;

		ComPtr<ID3D12StateObject> state_object;
		ComPtr<ID3D12StateObjectProperties> state_object_props;

	private:
		friend ShaderTable;
		friend CommandContext;
	};
}