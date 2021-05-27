#include "shader.h"
#include <d3dcompiler.h>
#include "internal/egx_internal.h"
#include <dxcapi.h>
#include <fstream>
#include <sstream>


namespace
{
	ComPtr<ID3DBlob> compileShader(const std::string& path, const std::string& entry_point, const std::string& version_target, const std::vector<D3D_SHADER_MACRO>& macro_list)
	{
		// Handle macros
		const D3D_SHADER_MACRO* macros = macro_list.data();
		if (macro_list.size() == 0)
			macros = nullptr;

		ComPtr<ID3DBlob> shader_buffer;
		ComPtr<ID3DBlob> error_message;

		std::wstring w_path(path.begin(), path.end());

#ifdef _DEBUG
		UINT shader_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT shader_flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
		//Compile the vertex shader code
		if (FAILED(
			D3DCompileFromFile(
				w_path.c_str(),
				macro_list.data(),
				D3D_COMPILE_STANDARD_FILE_INCLUDE,
				entry_point.c_str(),
				version_target.c_str(),
				shader_flags,
				0,
				&shader_buffer,
				&error_message
			)))
		{
			if (error_message.Get() == nullptr)
			{
				throw std::runtime_error("Could not find file " + path);
			}
#ifdef _DEBUG // Write error messages to log file
			else
			{
				char* compile_errors = (char*)(error_message->GetBufferPointer());
				unsigned long long buffer_size = error_message->GetBufferSize();

				std::string compile_errors_string;
				for (int i = 0; i < buffer_size; i++)
					compile_errors_string += compile_errors[i];

				eio::Console::Log("SHADER ERROR: " + compile_errors_string);
				throw std::runtime_error("Error compiling shader: " + path);
			}
#endif // _DEBUG
		}
		return shader_buffer;
	}
	 
    std::wstring convert_to_wstring(const std::string& s)
    {
        return std::wstring(s.begin(), s.end());
    }

    ComPtr<ID3DBlob> compileShader2(const std::string& filename, const std::wstring& entry_point, const std::string& target_string, const std::vector<DxcDefine>& macro_list)
    {
        auto wfilename = convert_to_wstring(filename);
        auto wtarget_string = convert_to_wstring(target_string);

        ComPtr<IDxcCompiler2> pcompiler;
        ComPtr<IDxcLibrary> plibrary;
        ComPtr<IDxcIncludeHandler> pinclude_handler;

        THROWIFFAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pcompiler)), "Failed to create DXC compiler");
        THROWIFFAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&plibrary)), "Failed to create DXC Library");

        // Open and read the file
        std::ifstream shader_file(filename);
        if (shader_file.fail())
        {
            throw std::runtime_error("Failed to open shader " + filename);
        }
        std::stringstream ss;
        ss << shader_file.rdbuf();
        std::string shader = ss.str();

        // Create blob from the string
        ComPtr<IDxcBlobEncoding> ptext_blob;
        THROWIFFAILED(plibrary->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &ptext_blob),
            "Failed to create blob from pinned");

        // Command line arguments
        const wchar_t* pargs[] =
        {
            //                      L"-Zpr",            // Row-major matrices
                                    L"-WX",             // Warnings as errors
                                    L"-enable-16bit-types",
            #ifdef _DEBUG
                                    L"-Zi",             // Debug info
                                    L"-Od",             // Disable optimizations
            #else
                                    L"-O3",             // Optimization level 3
            #endif
        };

        // Compile
        ComPtr<IDxcOperationResult> presult;
        THROWIFFAILED(pcompiler->Compile(ptext_blob.Get(), wfilename.c_str(), entry_point.c_str(), wtarget_string.c_str(), pargs, _countof(pargs), macro_list.data(), macro_list.size(), nullptr, &presult),
            "Failed to compile shader " + filename);

        // Verify the result
        HRESULT hr = 0;
        presult->GetStatus(&hr);
        if (FAILED(hr))
        {
            // Log error message
            ComPtr<IDxcBlobEncoding> perror;
            THROWIFFAILED(presult->GetErrorBuffer(&perror), "Failed to get errorbuffer");
            std::vector<char> error_chars(perror->GetBufferSize() + 1);
            memcpy(error_chars.data(), perror->GetBufferPointer(), perror->GetBufferSize());
            error_chars[perror->GetBufferSize()] = 0;
            std::string serror(error_chars.data());

            eio::Console::Log("Failed to compile " + filename);
            eio::Console::Log(serror);
            throw std::runtime_error("Failed to compile " + filename);
        }

        ComPtr<ID3DBlob> out;
        IDxcBlob** pblob = reinterpret_cast<IDxcBlob**>(out.GetAddressOf());
        THROWIFFAILED(presult->GetResult(pblob), "Failed to get result");
        return out;
    }

    std::vector<DxcDefine> macroToDxc(const egx::ShaderMacroList& macro_list)
    {
        std::vector<DxcDefine> out;
        for (const auto& e : macro_list.wmacros)
        {
            DxcDefine ne;
            ne.Name = e.first.c_str();
            ne.Value = e.second.c_str();
            out.push_back(ne);
        }
        return out;
    }

}

std::vector<D3D_SHADER_MACRO> egx::ShaderMacroList::getD3D() const
{
	std::vector<D3D_SHADER_MACRO> d3d_macros;
	for (auto& m : macros)
	{
		D3D_SHADER_MACRO macro;
		macro.Name = m.first.c_str();
		macro.Definition = m.second.c_str();
		d3d_macros.push_back(macro);
	}

	D3D_SHADER_MACRO macro{ nullptr, nullptr };
	d3d_macros.push_back(macro);

	return d3d_macros;
}

void egx::Shader::CompileVertexShader(const std::string& path, const ShaderMacroList& macro_list)
{
	shader_blob = compileShader(path, "VS", "vs_5_0", macro_list.getD3D());
}
void egx::Shader::CompileVertexShader2(const std::string& path, const ShaderMacroList& macro_list)
{
    shader_blob = compileShader2(path, L"VS", "vs_6_5", macroToDxc(macro_list));
}

void egx::Shader::CompilePixelShader(const std::string& path, const ShaderMacroList& macro_list)
{
    shader_blob = compileShader(path, "PS", "ps_5_0", macro_list.getD3D());
}
void egx::Shader::CompilePixelShader2(const std::string& path, const ShaderMacroList& macro_list)
{
	shader_blob = compileShader2(path, L"PS", "ps_6_5", macroToDxc(macro_list));
}

void egx::Shader::CompileComputeShader(const std::string& path, const ShaderMacroList& macro_list)
{
	shader_blob = compileShader(path, "CS", "cs_5_0", macro_list.getD3D());
}
void egx::Shader::CompileComputeShader2(const std::string& path, const ShaderMacroList& macro_list)
{
    shader_blob = compileShader2(path, L"CS", "cs_6_5", macroToDxc(macro_list));
}