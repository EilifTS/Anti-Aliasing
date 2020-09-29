#include "shader_library.h"
#include <dxcapi.h>
#include "../internal/egx_internal.h"
#include <fstream>
#include <sstream>
#include "../../io/console.h"

namespace
{
    std::wstring convert_to_wstring(const std::string& s)
    {
        return std::wstring(s.begin(), s.end());
    }

    ComPtr<ID3DBlob> compileLibrary(const std::string& filename, const std::string& target_string)
    {
        auto wfilename = convert_to_wstring(filename);
        auto wtarget_string = convert_to_wstring(target_string);

        ComPtr<IDxcCompiler> pcompiler;
        ComPtr<IDxcLibrary> plibrary;

        THROWIFFAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pcompiler)), "Failed to create DXC copiler");
        THROWIFFAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&plibrary)), "Failed to create DXC copiler");

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

        // Compile
        ComPtr<IDxcOperationResult> presult;
        THROWIFFAILED(pcompiler->Compile(ptext_blob.Get(), wfilename.c_str(), L"", wtarget_string.c_str(), nullptr, 0, nullptr, 0, nullptr, &presult),
            "Failed to compile shader" + filename);

        // Verify the result
        HRESULT hr = 0;
        presult->GetStatus(&hr);
        if (FAILED(hr))
        {
            // Log error message
            ComPtr<IDxcBlobEncoding> perror;
            THROWIFFAILED(presult->GetErrorBuffer(&perror), "Failed to get errorbuffer");
            std::wstringstream wss((const wchar_t*)perror->GetBufferPointer());
            auto wserror = wss.str();
            std::string serror(wserror.begin(), wserror.end());
            eio::Console::Log("Failed to compile " + filename);
            eio::Console::Log(serror);
            throw std::runtime_error("Failed to compile" + filename);
        }

        ComPtr<ID3DBlob> out;
        IDxcBlob** pblob = reinterpret_cast<IDxcBlob**>(out.GetAddressOf());
        THROWIFFAILED(presult->GetResult(pblob), "Failed to get result");
        return out;
    }
}


void egx::ShaderLibrary::Compile(const std::string& file_path)
{
    blob = compileLibrary(file_path, "lib_6_3");
}