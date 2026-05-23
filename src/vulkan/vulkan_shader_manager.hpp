#pragma once

#include "../common/shader_reflection.hpp"

#include <cstdint>
#include <string>
#include <vector>

struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace gpu
{
struct VulkanShader
{
    std::vector<uint32_t> spirv;
    ShaderReflection reflection;
};

class VulkanShaderManager
{
public:
    explicit VulkanShaderManager(char const* shader_path);
    ~VulkanShaderManager();
    void SetShaderPath(char const* shader_path);

    VulkanShader CompileShader(char const* filename, char const* entry_point, char const* shader_profile,
        std::vector<char const*> const& definitions = std::vector<char const*>(),
        char const* root_constants_name = "g_RootConstants");

private:
    std::string shader_path_;
    IDxcUtils* dxc_utils_ = nullptr;
    IDxcCompiler3* dxc_compiler_ = nullptr;
    IDxcIncludeHandler* dxc_include_handler_ = nullptr;
};

}  // namespace gpu
