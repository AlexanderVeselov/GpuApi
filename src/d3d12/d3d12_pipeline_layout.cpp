#include "d3d12_pipeline_layout.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_shader_manager.hpp"

#include <d3d12shader.h>

#include <cassert>
#include <stdexcept>
#include <string>

namespace gpu
{
namespace
{
D3D12_SHADER_VISIBILITY GetShaderVisibility(D3D12_SHADER_VERSION_TYPE version_type)
{
    switch (version_type)
    {
    case D3D12_SHVER_PIXEL_SHADER:
        return D3D12_SHADER_VISIBILITY_PIXEL;
    case D3D12_SHVER_VERTEX_SHADER:
        return D3D12_SHADER_VISIBILITY_VERTEX;
    case D3D12_SHVER_GEOMETRY_SHADER:
        return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case D3D12_SHVER_HULL_SHADER:
        return D3D12_SHADER_VISIBILITY_HULL;
    case D3D12_SHVER_DOMAIN_SHADER:
        return D3D12_SHADER_VISIBILITY_DOMAIN;
    case D3D12_SHVER_COMPUTE_SHADER:
        return D3D12_SHADER_VISIBILITY_ALL;
    default:
        assert(false && "D3D12PipelineLayout: unknown shader version type");
        return D3D12_SHADER_VISIBILITY_ALL;
    }
}

D3D12_DESCRIPTOR_RANGE_TYPE GetRangeType(D3D_SHADER_INPUT_TYPE type)
{
    switch (type)
    {
    case D3D_SIT_CBUFFER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case D3D_SIT_TBUFFER:
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case D3D_SIT_SAMPLER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    default:
        assert(false && "D3D12PipelineLayout: unsupported D3D shader input type");
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

D3D12Binding::ResourceType GetResourceType(D3D_SHADER_INPUT_TYPE type)
{
    switch (type)
    {
    case D3D_SIT_SAMPLER:
        return D3D12Binding::ResourceType::kSampler;
    case D3D_SIT_TEXTURE:
    case D3D_SIT_UAV_RWTYPED:
        return D3D12Binding::ResourceType::kImage;
    default:
        return D3D12Binding::ResourceType::kBuffer;
    }
}

D3D12_SHADER_VISIBILITY MergeVisibility(
    D3D12_SHADER_VISIBILITY lhs,
    D3D12_SHADER_VISIBILITY rhs)
{
    if (lhs == rhs)
    {
        return lhs;
    }

    return D3D12_SHADER_VISIBILITY_ALL;
}

std::string BindingName(D3D12Binding const& binding)
{
    return "(binding = " + std::to_string(binding.binding) +
        ", space = " + std::to_string(binding.space) + ")";
}
}

D3D12PipelineLayout::D3D12PipelineLayout(D3D12Device& device)
    : device_(device)
{
}

D3D12PipelineLayout::D3D12PipelineLayout(
    D3D12Device& device,
    std::vector<D3D12Shader const*> const& shaders)
    : device_(device)
{
    Build(shaders);
}

void D3D12PipelineLayout::Build(std::vector<D3D12Shader const*> const& shaders)
{
    Clear();

    for (D3D12Shader const* shader : shaders)
    {
        assert(shader && "D3D12PipelineLayout::Build: shader pointer is null");
        ReflectShader(*shader);
    }

    CreateRootSignature();
}

void D3D12PipelineLayout::Clear()
{
    root_signature_.Reset();
    root_parameters_.clear();
    descriptor_ranges_.clear();
    bindings_.clear();
}

bool D3D12PipelineLayout::HasBinding(uint32_t binding, uint32_t space) const
{
    for (D3D12Binding const& d3d12_binding : bindings_)
    {
        if (d3d12_binding.binding == binding && d3d12_binding.space == space)
        {
            return true;
        }
    }

    return false;
}

D3D12Binding const& D3D12PipelineLayout::FindBinding(
    uint32_t binding,
    uint32_t space) const
{
    for (D3D12Binding const& d3d12_binding : bindings_)
    {
        if (d3d12_binding.binding == binding && d3d12_binding.space == space)
        {
            return d3d12_binding;
        }
    }

    throw std::runtime_error("D3D12PipelineLayout::FindBinding: binding (binding = " +
        std::to_string(binding) + ", space = " + std::to_string(space) + ") was not found");
}

void D3D12PipelineLayout::ReflectShader(D3D12Shader const& shader)
{
    ID3D12ShaderReflection* reflection = shader.reflection.Get();
    assert(reflection && "D3D12PipelineLayout::ReflectShader: shader reflection is null");

    D3D12_SHADER_DESC shader_desc = {};
    ThrowIfFailed(reflection->GetDesc(&shader_desc));

    D3D12_SHADER_VISIBILITY visibility = GetShaderVisibility(
        static_cast<D3D12_SHADER_VERSION_TYPE>(D3D12_SHVER_GET_TYPE(shader_desc.Version)));

    for (uint32_t i = 0; i < shader_desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC resource_desc = {};
        ThrowIfFailed(reflection->GetResourceBindingDesc(i, &resource_desc));

        D3D12Binding binding;
        binding.name = resource_desc.Name ? resource_desc.Name : "";
        binding.binding = resource_desc.BindPoint;
        binding.space = resource_desc.Space;
        binding.type = GetResourceType(resource_desc.Type);
        binding.range_type = GetRangeType(resource_desc.Type);
        binding.descriptor_count = resource_desc.BindCount;
        binding.visibility = visibility;

        if (resource_desc.Type == D3D_SIT_CBUFFER && binding.name == "$Globals")
        {
            ID3D12ShaderReflectionConstantBuffer* cb =
                reflection->GetConstantBufferByName(binding.name.c_str());
            assert(cb && "D3D12PipelineLayout::ReflectShader: $Globals constant buffer was not found");

            D3D12_SHADER_BUFFER_DESC buffer_desc = {};
            ThrowIfFailed(cb->GetDesc(&buffer_desc));

            binding.descriptor_type = D3D12Binding::DescriptorType::kRootConstant;
            binding.num_32bit_values = buffer_desc.Size / 4;
        }
        else
        {
            binding.descriptor_type = D3D12Binding::DescriptorType::kDescriptorTable;
        }

        AddOrMergeBinding(binding);
    }
}

void D3D12PipelineLayout::AddOrMergeBinding(D3D12Binding const& binding)
{
    for (D3D12Binding& existing : bindings_)
    {
        if (existing.binding != binding.binding || existing.space != binding.space)
        {
            continue;
        }

        if (existing.type != binding.type ||
            existing.descriptor_type != binding.descriptor_type ||
            existing.range_type != binding.range_type ||
            existing.descriptor_count != binding.descriptor_count ||
            existing.num_32bit_values != binding.num_32bit_values)
        {
            throw std::runtime_error("D3D12PipelineLayout: incompatible shader bindings for " +
                BindingName(binding));
        }

        existing.visibility = MergeVisibility(existing.visibility, binding.visibility);
        return;
    }

    bindings_.push_back(binding);
}

void D3D12PipelineLayout::CreateRootSignature()
{
    descriptor_ranges_.clear();
    root_parameters_.clear();
    descriptor_ranges_.reserve(bindings_.size());
    root_parameters_.reserve(bindings_.size());

    for (D3D12Binding& binding : bindings_)
    {
        binding.root_parameter_index = static_cast<uint32_t>(root_parameters_.size());

        D3D12_ROOT_PARAMETER root_parameter = {};
        root_parameter.ShaderVisibility = binding.visibility;

        if (binding.descriptor_type == D3D12Binding::DescriptorType::kRootConstant)
        {
            root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
            root_parameter.Constants.ShaderRegister = binding.binding;
            root_parameter.Constants.RegisterSpace = binding.space;
            root_parameter.Constants.Num32BitValues = binding.num_32bit_values;
        }
        else
        {
            D3D12_DESCRIPTOR_RANGE descriptor_range = {};
            descriptor_range.RangeType = binding.range_type;
            descriptor_range.NumDescriptors = binding.descriptor_count;
            descriptor_range.BaseShaderRegister = binding.binding;
            descriptor_range.RegisterSpace = binding.space;
            descriptor_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            descriptor_ranges_.push_back(descriptor_range);

            root_parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            root_parameter.DescriptorTable.NumDescriptorRanges = 1;
            root_parameter.DescriptorTable.pDescriptorRanges = &descriptor_ranges_.back();
        }

        root_parameters_.push_back(root_parameter);
    }

    D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
    root_signature_desc.NumParameters = static_cast<UINT>(root_parameters_.size());
    root_signature_desc.pParameters = root_parameters_.data();
    root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3D10Blob> root_signature_blob;
    ComPtr<ID3D10Blob> root_signature_error_blob;
    HRESULT hr = D3D12SerializeRootSignature(
        &root_signature_desc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,
        &root_signature_blob,
        &root_signature_error_blob);
    if (FAILED(hr))
    {
        std::string error_message = "Failed to serialize D3D12 pipeline layout root signature";
        if (root_signature_error_blob)
        {
            std::string error_blob_message =
                static_cast<char*>(root_signature_error_blob->GetBufferPointer());
            if (!error_blob_message.empty() && error_blob_message.back() == '\n')
            {
                error_blob_message.pop_back();
            }

            error_message += ": " + error_blob_message;
        }

        throw D3D12Exception(error_message.c_str(), hr, __FILE__, __LINE__);
    }

    ThrowIfFailed(device_.GetD3D12Device()->CreateRootSignature(
        0,
        root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(),
        IID_PPV_ARGS(&root_signature_)));
}

} // namespace gpu
