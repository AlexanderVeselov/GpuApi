#pragma once

#include "d3d12_common.hpp"
#include <string>

class IDxcUtils;
class IDxcCompiler3;

namespace gpu
{

class D3D12ShaderManager
{
public:
    D3D12ShaderManager(char const* shader_path);
    void CompileShader(char const* filename);

private:
    std::string shader_path_;

    ComPtr<IDxcUtils> dxc_utils_;
    ComPtr<IDxcCompiler3> dxc_compiler_;
    ComPtr<IDxcIncludeHandler> dxc_include_handler_;
};

} // namespace gpu
