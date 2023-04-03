#include "d3d12_shader_manager.hpp"
#include "d3d12_exception.hpp"
#include "dxcapi.h"
#include <d3d12shader.h>
#include "../common/utils.hpp"

#include <vector>

namespace gpu
{
D3D12ShaderManager::D3D12ShaderManager(char const* shader_path)
    : shader_path_(shader_path)
{
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_utils_)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler_)));
    ThrowIfFailed(dxc_utils_->CreateDefaultIncludeHandler(&dxc_include_handler_));
}

D3D12Shader D3D12ShaderManager::CompileShader(char const* filename, char const* entry_point,
    char const* shader_profile, std::vector<char const*> const& definitions)
{
    ComPtr<IDxcBlobEncoding> dxc_source = nullptr;
    std::wstring w_filename = StringToWstring(filename);
    ThrowIfFailed(dxc_utils_->LoadFile(w_filename.c_str(), nullptr, &dxc_source));

    char* data = (char*)dxc_source->GetBufferPointer();

    DxcBuffer shader_source = {};
    shader_source.Ptr = dxc_source->GetBufferPointer();
    shader_source.Size = dxc_source->GetBufferSize();

    std::vector<LPCWSTR> shader_args;

    // Add entry point
    std::wstring w_entry_point = StringToWstring(entry_point);
    shader_args.push_back(L"-E");
    shader_args.push_back(w_entry_point.c_str());
    std::wstring w_shader_profile = StringToWstring(shader_profile);
    shader_args.push_back(L"-T");
    shader_args.push_back(w_shader_profile.c_str());

    // Add definitions
    std::vector<std::wstring> w_definitions;
    for (auto& definition : definitions)
    {
        w_definitions.push_back(StringToWstring(definition));
        shader_args.push_back(L"-D");
        shader_args.push_back(w_definitions.back().c_str());
    }

    shader_args.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);

#ifndef NDEBUG
    shader_args.push_back(DXC_ARG_DEBUG);
    shader_args.push_back(DXC_ARG_OPTIMIZATION_LEVEL0);
    shader_args.push_back(DXC_ARG_DEBUG_NAME_FOR_SOURCE);
#endif

    // Compile shader
    ComPtr<IDxcResult> dxc_result;
    ThrowIfFailed(dxc_compiler_->Compile(&shader_source, shader_args.data(), (UINT32)shader_args.size(),
        dxc_include_handler_, IID_PPV_ARGS(&dxc_result)));

    // Check compilation result
    ComPtr<IDxcBlobUtf8> dxc_error;
    dxc_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&dxc_error), nullptr);

    if (dxc_error && dxc_error->GetStringLength() > 0)
    {
        std::string error_message = "Failed to compile " + std::string(filename) + ":\n";
        error_message += (char*)dxc_error->GetBufferPointer();
        throw std::runtime_error(error_message.c_str());
    }

    // Get shader bytecode
    D3D12Shader shader = {};
    ThrowIfFailed(dxc_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader.dxc_blob), nullptr));

    // Get shader reflection
    ComPtr<IDxcBlob> dxc_reflection;
    ThrowIfFailed(dxc_result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&dxc_reflection), nullptr));

    // Create D3D12 reflection
    DxcBuffer reflection_buffer = {};
    reflection_buffer.Size = dxc_reflection->GetBufferSize();
    reflection_buffer.Ptr = dxc_reflection->GetBufferPointer();

    dxc_utils_->CreateReflection(&reflection_buffer, IID_PPV_ARGS(&shader.reflection));
    return shader;
}

} // namespace gpu
