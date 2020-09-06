#include "shader.h"
#include <d3dcompiler.h>
#include "internal/egx_internal.h"


namespace
{
	ComPtr<ID3DBlob> compileShader(const std::string& path, const std::string& entry_point, const std::string& version_target)
	{
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
				NULL,
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

}

void egx::Shader::CompileVertexShader(const std::string& path)
{
	shader_blob = compileShader(path, "VS", "vs_5_0");
}

void egx::Shader::CompilePixelShader(const std::string& path)
{
	shader_blob = compileShader(path, "PS", "ps_5_0");
}