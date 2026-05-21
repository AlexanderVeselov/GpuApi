#include "vulkan_shader_manager.hpp"

#include "../common/utils.hpp"
#include "vulkan_shader_reflection.hpp"

#define WIN32_LEAN_AND_MEAN
#include <ObjIdl.h>
#include <OleAuto.h>
#include <Unknwn.h>
#include <Windows.h>
#include <dxcapi.h>
#include <wrl.h>

#include <cstring>
#include <filesystem>
#include <stdexcept>

namespace gpu
{
using Microsoft::WRL::ComPtr;

namespace
{
void ThrowIfDxcFailed(HRESULT hr, char const* message)
{
    if (FAILED(hr))
    {
        throw std::runtime_error(message);
    }
}
}  // namespace

VulkanShaderManager::VulkanShaderManager(char const* shader_path) : shader_path_(shader_path)
{
    ThrowIfDxcFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxc_utils_)),
        "VulkanShaderManager: failed to create DXC utils");
    ThrowIfDxcFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxc_compiler_)),
        "VulkanShaderManager: failed to create DXC compiler");
    ThrowIfDxcFailed(dxc_utils_->CreateDefaultIncludeHandler(&dxc_include_handler_),
        "VulkanShaderManager: failed to create DXC include handler");
}

VulkanShaderManager::~VulkanShaderManager()
{
    if (dxc_include_handler_)
    {
        dxc_include_handler_->Release();
    }

    if (dxc_compiler_)
    {
        dxc_compiler_->Release();
    }

    if (dxc_utils_)
    {
        dxc_utils_->Release();
    }
}

void VulkanShaderManager::SetShaderPath(char const* shader_path)
{
    shader_path_ = shader_path ? shader_path : "";
}

VulkanShader VulkanShaderManager::CompileShader(char const* filename, char const* entry_point,
    char const* shader_profile, std::vector<char const*> const& definitions)
{
    ComPtr<IDxcBlobEncoding> dxc_source = nullptr;
    std::filesystem::path shader_file = filename;
    if (!shader_path_.empty() && shader_file.is_relative())
    {
        shader_file = std::filesystem::path(shader_path_) / shader_file;
    }

    std::wstring w_filename = shader_file.wstring();
    ThrowIfDxcFailed(dxc_utils_->LoadFile(w_filename.c_str(), nullptr, &dxc_source),
        "VulkanShaderManager::CompileShader: failed to load shader file");

    DxcBuffer shader_source = {};
    shader_source.Ptr = dxc_source->GetBufferPointer();
    shader_source.Size = dxc_source->GetBufferSize();
    shader_source.Encoding = DXC_CP_UTF8;

    std::vector<LPCWSTR> shader_args;
    std::wstring w_entry_point = StringToWstring(entry_point);
    std::wstring w_shader_profile = StringToWstring(shader_profile);

    shader_args.push_back(L"-E");
    shader_args.push_back(w_entry_point.c_str());
    shader_args.push_back(L"-T");
    shader_args.push_back(w_shader_profile.c_str());
    shader_args.push_back(L"-spirv");
    shader_args.push_back(L"-fspv-target-env=vulkan1.2");
    shader_args.push_back(L"-fspv-extension=SPV_EXT_descriptor_indexing");
    shader_args.push_back(L"-fspv-extension=SPV_KHR_ray_query");
    shader_args.push_back(L"-fspv-extension=SPV_KHR_ray_tracing");
    shader_args.push_back(L"-fvk-use-dx-layout");
    shader_args.push_back(L"-Zpr");
    shader_args.push_back(L"-D");
    shader_args.push_back(L"IMAGE_FORMAT(format)=[[vk::image_format(format)]]");

    std::wstring w_shader_path;
    if (!shader_path_.empty())
    {
        w_shader_path = std::filesystem::path(shader_path_).wstring();
        shader_args.push_back(L"-I");
        shader_args.push_back(w_shader_path.c_str());
    }

    std::vector<std::wstring> w_definitions;
    for (char const* definition : definitions)
    {
        w_definitions.push_back(StringToWstring(definition));
        shader_args.push_back(L"-D");
        shader_args.push_back(w_definitions.back().c_str());
    }

    shader_args.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);

#ifndef NDEBUG
    shader_args.push_back(DXC_ARG_DEBUG);
    shader_args.push_back(DXC_ARG_OPTIMIZATION_LEVEL0);
#endif

    ComPtr<IDxcResult> dxc_result;
    ThrowIfDxcFailed(dxc_compiler_->Compile(&shader_source, shader_args.data(), static_cast<UINT32>(shader_args.size()),
                         dxc_include_handler_, IID_PPV_ARGS(&dxc_result)),
        "VulkanShaderManager::CompileShader: DXC compile call failed");

    ComPtr<IDxcBlobUtf8> dxc_error;
    dxc_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&dxc_error), nullptr);
    if (dxc_error && dxc_error->GetStringLength() > 0)
    {
        std::string error_message = "Failed to compile Vulkan shader ";
        error_message += filename;
        error_message += ":\n";
        error_message += static_cast<char const*>(dxc_error->GetBufferPointer());
        throw std::runtime_error(error_message);
    }

    ComPtr<IDxcBlob> dxc_object;
    ThrowIfDxcFailed(dxc_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&dxc_object), nullptr),
        "VulkanShaderManager::CompileShader: failed to get SPIR-V object");

    if (dxc_object->GetBufferSize() % sizeof(uint32_t) != 0)
    {
        throw std::runtime_error("VulkanShaderManager::CompileShader: SPIR-V blob is misaligned");
    }

    VulkanShader shader;
    shader.spirv.resize(dxc_object->GetBufferSize() / sizeof(uint32_t));
    std::memcpy(shader.spirv.data(), dxc_object->GetBufferPointer(), dxc_object->GetBufferSize());
    shader.reflection = BuildVulkanShaderReflection(shader.spirv);

    return shader;
}

}  // namespace gpu
