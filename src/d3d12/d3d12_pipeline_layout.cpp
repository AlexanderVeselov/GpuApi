#include "d3d12_pipeline_layout.hpp"
#include "d3d12_device.hpp"
#include "d3d12_exception.hpp"
#include "d3d12_shader_manager.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <tuple>

namespace gpu
{
namespace
{
D3D12_SHADER_VISIBILITY GetShaderVisibility(uint32_t stage_mask)
{
    if ((stage_mask & (stage_mask - 1)) != 0)
    {
        return D3D12_SHADER_VISIBILITY_ALL;
    }

    switch (static_cast<ShaderStage>(stage_mask))
    {
    case ShaderStage::kPixel:
        return D3D12_SHADER_VISIBILITY_PIXEL;
    case ShaderStage::kVertex:
        return D3D12_SHADER_VISIBILITY_VERTEX;
    case ShaderStage::kGeometry:
        return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case ShaderStage::kHull:
        return D3D12_SHADER_VISIBILITY_HULL;
    case ShaderStage::kDomain:
        return D3D12_SHADER_VISIBILITY_DOMAIN;
    case ShaderStage::kCompute:
        return D3D12_SHADER_VISIBILITY_ALL;
    default:
        assert(false && "D3D12PipelineLayout: unknown shader stage");
        return D3D12_SHADER_VISIBILITY_ALL;
    }
}

D3D12_DESCRIPTOR_RANGE_TYPE GetRangeType(ShaderDescriptorRangeType type)
{
    switch (type)
    {
    case ShaderDescriptorRangeType::kCBV:
        return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    case ShaderDescriptorRangeType::kSRV:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    case ShaderDescriptorRangeType::kUAV:
        return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    case ShaderDescriptorRangeType::kSampler:
        return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
    default:
        assert(false && "D3D12PipelineLayout: unsupported shader descriptor range type");
        return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

D3D12Binding::ResourceType GetResourceType(ShaderResourceType type)
{
    switch (type)
    {
    case ShaderResourceType::kBuffer:
        return D3D12Binding::ResourceType::kBuffer;
    case ShaderResourceType::kImage:
        return D3D12Binding::ResourceType::kImage;
    case ShaderResourceType::kSampler:
        return D3D12Binding::ResourceType::kSampler;
    case ShaderResourceType::kAccelerationStructure:
        return D3D12Binding::ResourceType::kAccelerationStructure;
    default:
        return D3D12Binding::ResourceType::kBuffer;
    }
}

D3D12Binding::DescriptorType GetDescriptorType(ShaderDescriptorType type)
{
    switch (type)
    {
    case ShaderDescriptorType::kRootConstant:
        return D3D12Binding::DescriptorType::kRootConstant;
    case ShaderDescriptorType::kDescriptorTable:
        return D3D12Binding::DescriptorType::kDescriptorTable;
    default:
        return D3D12Binding::DescriptorType::kDescriptorTable;
    }
}

D3D12_SHADER_VISIBILITY MergeVisibility(D3D12_SHADER_VISIBILITY lhs, D3D12_SHADER_VISIBILITY rhs)
{
    if (lhs == rhs)
    {
        return lhs;
    }

    return D3D12_SHADER_VISIBILITY_ALL;
}

std::string BindingName(D3D12Binding const& binding)
{
    return "(binding = " + std::to_string(binding.binding) + ", space = " + std::to_string(binding.space) + ")";
}
}  // namespace

D3D12PipelineLayout::D3D12PipelineLayout(D3D12Device& device) : device_(device) {}

D3D12PipelineLayout::D3D12PipelineLayout(D3D12Device& device, std::vector<D3D12Shader const*> const& shaders)
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
        AddShaderReflection(shader->reflection);
    }

    // Reflection order is not part of the pipeline layout ABI. Canonicalize bindings before assigning root parameter
    // indices so hot reload compatibility does not depend on compiler/reflection iteration order.
    SortBindings();
    CreateRootSignature();
}

void D3D12PipelineLayout::Clear()
{
    root_signature_.Reset();
    root_parameters_.clear();
    descriptor_ranges_.clear();
    bindings_.clear();
}

bool D3D12PipelineLayout::IsCompatibleWith(D3D12PipelineLayout const& other) const
{
    if (bindings_.size() != other.bindings_.size())
    {
        return false;
    }

    for (size_t i = 0; i < bindings_.size(); ++i)
    {
        D3D12Binding const& lhs = bindings_[i];
        D3D12Binding const& rhs = other.bindings_[i];
        if (lhs.binding != rhs.binding || lhs.space != rhs.space || lhs.root_parameter_index != rhs.root_parameter_index
            || lhs.type != rhs.type || lhs.descriptor_type != rhs.descriptor_type || lhs.range_type != rhs.range_type
            || lhs.descriptor_count != rhs.descriptor_count || lhs.num_32bit_values != rhs.num_32bit_values
            || lhs.visibility != rhs.visibility)
        {
            return false;
        }
    }

    return true;
}

bool D3D12PipelineLayout::HasBinding(uint32_t binding, uint32_t space) const
{
    for (D3D12Binding const& d3d12_binding : bindings_)
    {
        if (d3d12_binding.binding == binding && d3d12_binding.space == space
            && d3d12_binding.descriptor_type == D3D12Binding::DescriptorType::kDescriptorTable)
        {
            return true;
        }
    }

    return false;
}

D3D12Binding const& D3D12PipelineLayout::FindBinding(uint32_t binding, uint32_t space) const
{
    for (D3D12Binding const& d3d12_binding : bindings_)
    {
        if (d3d12_binding.binding == binding && d3d12_binding.space == space
            && d3d12_binding.descriptor_type == D3D12Binding::DescriptorType::kDescriptorTable)
        {
            return d3d12_binding;
        }
    }

    throw std::runtime_error("D3D12PipelineLayout::FindBinding: binding (binding = " + std::to_string(binding)
        + ", space = " + std::to_string(space) + ") was not found");
}

D3D12Binding const& D3D12PipelineLayout::FindRootConstants() const
{
    D3D12Binding const* result = nullptr;
    for (D3D12Binding const& d3d12_binding : bindings_)
    {
        if (d3d12_binding.descriptor_type == D3D12Binding::DescriptorType::kRootConstant)
        {
            if (result)
            {
                throw std::runtime_error("D3D12PipelineLayout::FindRootConstants: multiple root constants found");
            }
            result = &d3d12_binding;
        }
    }

    if (!result)
    {
        throw std::runtime_error("D3D12PipelineLayout::FindRootConstants: root constant was not found");
    }
    return *result;
}

void D3D12PipelineLayout::AddShaderReflection(ShaderReflection const& reflection)
{
    for (ShaderBinding const& shader_binding : reflection.bindings)
    {
        D3D12Binding binding;
        binding.name = shader_binding.name;
        binding.binding = shader_binding.binding;
        binding.space = shader_binding.space;
        binding.type = GetResourceType(shader_binding.resource_type);
        binding.descriptor_type = GetDescriptorType(shader_binding.descriptor_type);
        binding.range_type = GetRangeType(shader_binding.range_type);
        binding.descriptor_count = shader_binding.descriptor_count;
        binding.num_32bit_values = shader_binding.num_32bit_values;
        binding.visibility = GetShaderVisibility(shader_binding.stage_mask);

        AddOrMergeBinding(binding);
    }
}

void D3D12PipelineLayout::AddOrMergeBinding(D3D12Binding const& binding)
{
    for (D3D12Binding& existing : bindings_)
    {
        if (existing.binding != binding.binding || existing.space != binding.space
            || existing.descriptor_type != binding.descriptor_type)
        {
            continue;
        }

        if (existing.type != binding.type || existing.range_type != binding.range_type
            || existing.descriptor_count != binding.descriptor_count
            || existing.num_32bit_values != binding.num_32bit_values)
        {
            throw std::runtime_error("D3D12PipelineLayout: incompatible shader bindings for " + BindingName(binding));
        }

        existing.visibility = MergeVisibility(existing.visibility, binding.visibility);
        return;
    }

    bindings_.push_back(binding);
}

void D3D12PipelineLayout::SortBindings()
{
    std::sort(bindings_.begin(), bindings_.end(),
        [](D3D12Binding const& lhs, D3D12Binding const& rhs)
        {
            return std::tie(lhs.space, lhs.binding, lhs.descriptor_type, lhs.range_type, lhs.type)
                < std::tie(rhs.space, rhs.binding, rhs.descriptor_type, rhs.range_type, rhs.type);
        });
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
    HRESULT hr = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob,
        &root_signature_error_blob);
    if (FAILED(hr))
    {
        std::string error_message = "Failed to serialize D3D12 pipeline layout root signature";
        if (root_signature_error_blob)
        {
            std::string error_blob_message = static_cast<char*>(root_signature_error_blob->GetBufferPointer());
            if (!error_blob_message.empty() && error_blob_message.back() == '\n')
            {
                error_blob_message.pop_back();
            }

            error_message += ": " + error_blob_message;
        }

        throw D3D12Exception(error_message.c_str(), hr, __FILE__, __LINE__);
    }

    ThrowIfFailed(device_.GetD3D12Device()->CreateRootSignature(0, root_signature_blob->GetBufferPointer(),
        root_signature_blob->GetBufferSize(), IID_PPV_ARGS(&root_signature_)));
}

}  // namespace gpu
