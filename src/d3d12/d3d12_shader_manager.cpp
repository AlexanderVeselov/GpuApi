#include "d3d12_shader_manager.hpp"
#include "d3d12_exception.hpp"
#include "dxcapi.h"
#include "../common/utils.hpp"

#include <vector>

namespace gpu
{
D3D12ShaderManager::D3D12ShaderManager(char const* shader_path)
    : shader_path_(shader_path)
{
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_utils_)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler_)));
}

void D3D12ShaderManager::CompileShader(char const* filename)
{
    IDxcBlobEncoding* dxc_source = nullptr;
    std::wstring w_filename = StringToWstring(filename);
    ThrowIfFailed(dxc_utils_->LoadFile(w_filename.c_str(), nullptr, &dxc_source));

    DxcBuffer shader_source = {};
    shader_source.Ptr = dxc_source->GetBufferPointer();
    shader_source.Size = dxc_source->GetBufferSize();

    IDxcResult* dxc_result = nullptr;
    std::vector<LPCWSTR> shader_args;

    dxc_compiler_->Compile(&shader_source, shader_args.data(), (uint32_t)shader_args.size(), dxc_include_handler_, IID_PPV_ARGS(&dxc_result));

}

} // namespace gpu
