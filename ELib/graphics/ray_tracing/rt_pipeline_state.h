#pragma once
#include "../internal/egx_common.h"
#include "shader_library.h"
#include <vector>

namespace egx
{
	class RTPipelineState
	{
	public:
		void AddLibrary(ShaderLibrary& lib, const std::vector<std::string>& exports);
		void AddHitGroup(const std::string& hit_group_name, const std::string& closest_hit_symbol);
		void AddHitGroup(const std::string& hit_group_name, const std::string& closest_hit_symbol, const std::string& any_hit_symbol, const std::string& intersection_symbol);
		void AddRootSignatureAssociation(RootSignature& rs, const std::vector<std::string>& symbols);
		void SetMaxPayloadSize(int new_value) { max_payload_size = new_value; };
		void SetMaxAttributeSize(int new_value) { max_attribute_size = new_value; };
		void SetMaxRecursionDepth(int new_value) { maz_recursion_depth = new_value; };

		void Finalize(Device& dev);

	private:
		void buildShaderExportList(std::vector<std::wstring>& exported_symbols);

	private:
		struct library
		{
			library(ShaderLibrary& lib, const std::vector<std::string>& exports);

			ComPtr<ID3DBlob> byte_code;
			D3D12_DXIL_LIBRARY_DESC lib_desc;
			std::vector<D3D12_EXPORT_DESC> export_descs;
			std::vector<std::wstring> export_symbols;
		};

		struct hitGroup
		{
			hitGroup(const std::string& hit_group_name,
					 const std::string& closest_hit_symbol,
					 const std::string& any_hit_symbol,
					 const std::string& intersection_symbol);

			std::wstring hit_group_name;
			std::wstring closest_hit_symbol;
			std::wstring any_hit_symbol;
			std::wstring intersection_symbol;
			D3D12_HIT_GROUP_DESC desc = {};

		};

		struct rsAssociation
		{
			rsAssociation(RootSignature& rs, const std::vector<std::string>& symbols);

			ComPtr<ID3D12RootSignature> rs;
			std::vector<std::wstring> symbols;
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association;
		};

		std::vector<library> libraries;
		std::vector<hitGroup> hit_groups;
		std::vector<rsAssociation> rs_associations = {};
		int max_payload_size;
		int max_attribute_size;
		int max_recursion_depth;

		ComPtr<ID3D12StateObject> state_object;
	};
}