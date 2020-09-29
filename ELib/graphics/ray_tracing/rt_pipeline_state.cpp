#include "rt_pipeline_state.h"
#include "../root_signature.h"
#include "../device.h"
#include "../internal/egx_internal.h"
#include <unordered_set>
#include <stdexcept>
#include <assert.h>

void egx::RTPipelineState::AddLibrary(ShaderLibrary& lib, const std::vector<std::string>& exports)
{
	libraries.emplace_back(library(lib, exports));
}


void egx::RTPipelineState::AddHitGroup(const std::string& hit_group_name, const std::string& closest_hit_symbol)
{
	hit_groups.emplace_back(hitGroup(hit_group_name, closest_hit_symbol, "", ""));
}
void egx::RTPipelineState::AddHitGroup(const std::string& hit_group_name, const std::string& closest_hit_symbol, const std::string& any_hit_symbol, const std::string& intersection_symbol)
{
	hit_groups.emplace_back(hitGroup(hit_group_name, closest_hit_symbol, any_hit_symbol, intersection_symbol));
}

void egx::RTPipelineState::AddRootSignatureAssociation(RootSignature& rs, const std::vector<std::string>& symbols)
{
	rs_associations.emplace_back(rsAssociation(rs, symbols));
}

void egx::RTPipelineState::Finalize(Device& dev)
{
	int subobject_count =
		(int)libraries.size() +				// DXIL libs
		(int)hit_groups.size() +			// Hit groups
		1 +									// Shader config
		1 +									// Shader payload assoc
		2 * (int)rs_associations.size() +	// RS sig dec + assoc
		1;									// Pipeline subobject

	std::vector<D3D12_STATE_SUBOBJECT> subobjects(subobject_count);
	int current_index = 0;

	// Create subobjects for libraries
	for (const auto& lib : libraries)
	{
		D3D12_STATE_SUBOBJECT lib_subobject = {};
		lib_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		lib_subobject.pDesc = &lib.lib_desc;
		subobjects[current_index++] = lib_subobject;
	}

	// Create subobjects for hit groups
	for (const auto& group : hit_groups)
	{
		D3D12_STATE_SUBOBJECT hg_subobject = {};
		hg_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		hg_subobject.pDesc = &group.desc;
		subobjects[current_index++] = hg_subobject;
	}

	// Create subobject for shader config
	D3D12_RAYTRACING_SHADER_CONFIG shader_desc = {};
	shader_desc.MaxPayloadSizeInBytes = (unsigned int)max_payload_size;
	shader_desc.MaxAttributeSizeInBytes = (unsigned int)max_attribute_size;

	D3D12_STATE_SUBOBJECT shader_config_subobject = {};
	shader_config_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	shader_config_subobject.pDesc = &shader_desc;

	subobjects[current_index++] = shader_config_subobject;

	// Build list of all symbols for ray generation, miss and hit groups
	// And create associations with payload definition
	std::vector<std::wstring> exported_symbols = {};
	buildShaderExportList(exported_symbols);

	// Get pointers
	std::vector<LPCWSTR> exported_symbols_pointers(exported_symbols.size());
	for (int i = 0; i < (int)exported_symbols.size(); i++)
		exported_symbols_pointers[i] = exported_symbols[i].c_str();
	const WCHAR** shader_exports = exported_symbols_pointers.data();

	// Create association
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shader_payload_assoc = {};
	shader_payload_assoc.NumExports = (unsigned int)exported_symbols.size();
	shader_payload_assoc.pExports = shader_exports;
	shader_payload_assoc.pSubobjectToAssociate = &subobjects[(long long)current_index - 1];

	D3D12_STATE_SUBOBJECT shader_payload_assoc_subobject = {};
	shader_payload_assoc_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
	shader_payload_assoc_subobject.pDesc = &shader_payload_assoc;
	subobjects[current_index++] = shader_payload_assoc_subobject;

	// Create subobjects for root signature definition and association
	for (rsAssociation& assoc : rs_associations)
	{
		D3D12_STATE_SUBOBJECT rs_subobject = {};
		rs_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		rs_subobject.pDesc = assoc.rs.Get();
		
		subobjects[current_index++] = rs_subobject;

		// Get exports
		std::vector<LPCWSTR> export_pointers(assoc.symbols.size());
		for (int i = 0; i < (int)assoc.symbols.size(); i++)
			export_pointers[i] = assoc.symbols[i].c_str();
		const WCHAR** rs_exports = export_pointers.data();

		assoc.association.NumExports = (unsigned int)export_pointers.size();
		assoc.association.pExports = rs_exports;
		assoc.association.pSubobjectToAssociate = &subobjects[(long long)current_index - 1];

		D3D12_STATE_SUBOBJECT rs_assoc_subobject = {};
		rs_assoc_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		rs_assoc_subobject.pDesc = &assoc.association;

		subobjects[current_index++] = rs_assoc_subobject;
	}

	// Create pipeline subobject
	D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
	pipeline_config.MaxTraceRecursionDepth = max_recursion_depth;

	D3D12_STATE_SUBOBJECT pipeline_config_subobject = {};
	pipeline_config_subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	pipeline_config_subobject.pDesc = &pipeline_config;
	subobjects[current_index++] = pipeline_config_subobject;

	assert(current_index == subobject_count);

	// Create the pipeline state object
	D3D12_STATE_OBJECT_DESC pipeline_desc = {};
	pipeline_desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	pipeline_desc.NumSubobjects = (unsigned int)subobject_count;
	pipeline_desc.pSubobjects = subobjects.data();

	THROWIFFAILED(dev.device->CreateStateObject(&pipeline_desc, IID_PPV_ARGS(&state_object)), "Failed to create ray tracing pipeline state object");
}

void egx::RTPipelineState::buildShaderExportList(std::vector<std::wstring>& exported_symbols)
{
	// Get all symbols from librarie
	std::unordered_set<std::wstring> exports;

	// Add all the symbols from the libraries
	for (const library& lib : libraries)
	{
		for (const auto& exportName : lib.export_symbols)
		{
#ifdef _DEBUG
			// Sanity check in debug mode: check that no name is exported more than once
			if (exports.find(exportName) != exports.end())
			{
				throw std::runtime_error("Multiple definition of a symbol in the imported DXIL libraries");
			}
#endif
			exports.insert(exportName);
		}
	}

#ifdef _DEBUG
	// Sanity check in debug mode: verify that the hit groups do not reference an unknown shader name
	std::unordered_set<std::wstring> all_exports = exports;

	for (const auto& hitGroup : hit_groups)
	{
		if (!hitGroup.any_hit_symbol.empty() && exports.find(hitGroup.any_hit_symbol) == exports.end())
			throw std::runtime_error("Any hit symbol not found in the imported DXIL libraries");

		if (!hitGroup.closest_hit_symbol.empty() &&
			exports.find(hitGroup.closest_hit_symbol) == exports.end())
			throw std::runtime_error("Closest hit symbol not found in the imported DXIL libraries");

		if (!hitGroup.intersection_symbol.empty() &&
			exports.find(hitGroup.intersection_symbol) == exports.end())
			throw std::runtime_error("Intersection symbol not found in the imported DXIL libraries");

		all_exports.insert(hitGroup.hit_group_name);
	}
	// Sanity check in debug mode: verify that the root signature associations do not reference an
	// unknown shader or hit group name
	for (const auto& assoc : rs_associations)
	{
		for (const auto& symb : assoc.symbols)
		{
			if (!symb.empty() && all_exports.find(symb) == all_exports.end())
			{
				throw std::runtime_error("Root association symbol not found in the "
					"imported DXIL libraries and hit group names");
			}
		}
	}
#endif

	// Go through all hit groups and remove the symbols corresponding to intersection, any hit and
	// closest hit shaders from the symbol set
	for (const auto& hitGroup : hit_groups)
	{
		if (!hitGroup.any_hit_symbol.empty())
			exports.erase(hitGroup.any_hit_symbol);
		if (!hitGroup.closest_hit_symbol.empty())
			exports.erase(hitGroup.closest_hit_symbol);
		if (!hitGroup.intersection_symbol.empty())
			exports.erase(hitGroup.intersection_symbol);
		exports.insert(hitGroup.hit_group_name);
	}

	for (const auto& name : exports)
	{
		exported_symbols.push_back(name);
	}
}

egx::RTPipelineState::library::library(ShaderLibrary& lib, const std::vector<std::string>& exports)
	: export_descs(exports.size()), export_symbols(exports.size()), byte_code(lib.blob)
{
	for (int i = 0; i < (int)exports.size(); i++)
	{
		export_symbols[i] = std::wstring(exports[i].begin(), exports[i].end());

		export_descs[i] = {};
		export_descs[i].Name = export_symbols[i].c_str();
		export_descs[i].ExportToRename = nullptr;
		export_descs[i].Flags = D3D12_EXPORT_FLAG_NONE;
	}

	// Create library descriptor
	lib_desc.DXILLibrary.BytecodeLength = lib.blob->GetBufferSize();
	lib_desc.DXILLibrary.pShaderBytecode = lib.blob->GetBufferPointer();
	lib_desc.NumExports = (unsigned int)exports.size();
	lib_desc.pExports = export_descs.data();
}

egx::RTPipelineState::hitGroup::hitGroup(
	const std::string& hit_group_name,
	const std::string& closest_hit_symbol,
	const std::string& any_hit_symbol,
	const std::string& intersection_symbol)
{
	this->hit_group_name		= std::wstring(hit_group_name.begin(),		hit_group_name.end());
	this->closest_hit_symbol	= std::wstring(closest_hit_symbol.begin(),	closest_hit_symbol.end());
	this->any_hit_symbol		= std::wstring(any_hit_symbol.begin(),		any_hit_symbol.end());
	this->intersection_symbol	= std::wstring(intersection_symbol.begin(), intersection_symbol.end());

	desc.HitGroupExport = this->hit_group_name.c_str();
	desc.ClosestHitShaderImport = this->closest_hit_symbol.empty() ? nullptr : this->closest_hit_symbol.c_str();
	desc.AnyHitShaderImport = this->any_hit_symbol.empty() ? nullptr : this->any_hit_symbol.c_str();
	desc.IntersectionShaderImport = this->intersection_symbol.empty() ? nullptr : this->intersection_symbol.c_str();
}

egx::RTPipelineState::rsAssociation::rsAssociation(RootSignature& rs, const std::vector<std::string>& symbols)
	: rs(rs.root_signature), symbols(symbols.size()), association()
{
	for (int i = 0; i < (int)symbols.size(); i++)
		this->symbols[i] = std::wstring(symbols[i].begin(), symbols[i].end());
}