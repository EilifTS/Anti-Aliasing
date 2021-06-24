#pragma once
#include "internal/egx_common.h"
#include <string>
#include <unordered_map>

namespace egx
{
	class ShaderMacroList
	{
	public:
		ShaderMacroList() : macros(), wmacros() {};

		inline void Clear() { macros.clear(); wmacros.clear(); };
		inline void SetMacro(const std::string& name, const std::string& definition) { macros[name] = definition; wmacros[std::wstring(name.begin(), name.end())] = std::wstring(definition.begin(), definition.end()); };
		inline void RemoveMacro(const std::string& name) { macros.erase(name); wmacros[std::wstring(name.begin(), name.end())].erase(); };

		std::unordered_map<std::wstring, std::wstring> wmacros; // Bad hack for new compilers
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
		inline void CompileVertexShader2(const std::string& path) { CompileVertexShader2(path, ShaderMacroList()); };
		void CompileVertexShader(const std::string& path, const ShaderMacroList& macro_list);
		void CompileVertexShader2(const std::string& path, const ShaderMacroList& macro_list, bool use_native_16bit_ops = true);
		inline void CompilePixelShader(const std::string& path) { CompilePixelShader(path, ShaderMacroList()); };
		inline void CompilePixelShader2(const std::string& path) { CompilePixelShader2(path, ShaderMacroList()); };
		void CompilePixelShader(const std::string& path, const ShaderMacroList& macro_list);
		void CompilePixelShader2(const std::string& path, const ShaderMacroList& macro_list, bool use_native_16bit_ops = true);
		inline void CompileComputeShader(const std::string& path) { CompileComputeShader(path, ShaderMacroList()); };
		inline void CompileComputeShader2(const std::string& path) { CompileComputeShader2(path, ShaderMacroList()); };
		void CompileComputeShader(const std::string& path, const ShaderMacroList& macro_list);
		void CompileComputeShader2(const std::string& path, const ShaderMacroList& macro_list, bool use_native_16bit_ops = true);
	private:
		ComPtr<ID3DBlob> shader_blob;

		friend PipelineState;
		friend ComputePipelineState;
	};
}