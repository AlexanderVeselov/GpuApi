#pragma once

#include "../common/shader_reflection.hpp"
#include "d3d12_common.hpp"

#include <string>
#include <vector>

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcBlob;
struct IDxcIncludeHandler;

namespace gpu
{
struct D3D12Shader
{
    ComPtr<IDxcBlob> dxc_blob;
    ShaderReflection reflection;
};

class D3D12ShaderManager
{
public:
    D3D12ShaderManager(char const* shader_path);
    void SetShaderPath(char const* shader_path);
    D3D12Shader CompileShader(char const* filename, char const* entry_point, char const* shader_profile,
        std::vector<char const*> const& definitions = std::vector<char const*>(),
        char const* root_constants_name = "g_RootConstants");

private:
    std::string shader_path_;

    IDxcUtils* dxc_utils_;
    IDxcCompiler3* dxc_compiler_;
    IDxcIncludeHandler* dxc_include_handler_;
};

}  // namespace gpu
