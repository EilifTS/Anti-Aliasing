#pragma once
#include "internal/egx_common.h"
#include <string>
#include <unordered_map>

namespace egx
{
	class ShaderMacroList
	{
	public:
		ShaderMacroList() : macros() {};

		inline void Clear() { macros.clear(); };
		inline void SetMacro(const std::string& name, const std::string& definition) { macros[name] = definition; };
		inline void RemoveMacro(const std::string& name) { macros.erase(name); };

	private:
		std::vector<D3D_SHADER_MACRO> getD3D() const;

	private:
		std::unordered_map<std::string, std::string> macros;
		friend Shader;
	};

	class Shader
	{
	public:

		inline void CompileVertexShader(const std::string& path) { CompileVertexShader(path, ShaderMacroList()); };
		void CompileVertexShader(const std::string& path, const ShaderMacroList& macro_list);
		inline void CompilePixelShader(const std::string& path) { CompilePixelShader(path, ShaderMacroList()); };
		void CompilePixelShader(const std::string& path, const ShaderMacroList& macro_list);
	private:
		ComPtr<ID3DBlob> shader_blob;

		friend PipelineState;
	};
}