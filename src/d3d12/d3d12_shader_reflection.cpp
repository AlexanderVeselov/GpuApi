#include "d3d12_shader_reflection.hpp"

#include <d3d12shader.h>

#include <cassert>
#include <stdexcept>
#include <string_view>

namespace gpu
{
namespace
{
ShaderStage GetShaderStage(D3D12_SHADER_VERSION_TYPE version_type)
{
    switch (version_type)
    {
    case D3D12_SHVER_PIXEL_SHADER:
        return ShaderStage::kPixel;
    case D3D12_SHVER_VERTEX_SHADER:
        return ShaderStage::kVertex;
    case D3D12_SHVER_GEOMETRY_SHADER:
        return ShaderStage::kGeometry;
    case D3D12_SHVER_HULL_SHADER:
        return ShaderStage::kHull;
    case D3D12_SHVER_DOMAIN_SHADER:
        return ShaderStage::kDomain;
    case D3D12_SHVER_COMPUTE_SHADER:
        return ShaderStage::kCompute;
    default:
        assert(false && "BuildD3D12ShaderReflection: unknown shader stage");
        return ShaderStage::kVertex;
    }
}

ShaderDescriptorRangeType GetRangeType(D3D_SHADER_INPUT_TYPE type)
{
    switch (type)
    {
    case D3D_SIT_CBUFFER:
        return ShaderDescriptorRangeType::kCBV;
    case D3D_SIT_TBUFFER:
    case D3D_SIT_TEXTURE:
    case D3D_SIT_STRUCTURED:
    case D3D_SIT_BYTEADDRESS:
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
        return ShaderDescriptorRangeType::kSRV;
    case D3D_SIT_SAMPLER:
        return ShaderDescriptorRangeType::kSampler;
    case D3D_SIT_UAV_RWTYPED:
    case D3D_SIT_UAV_RWSTRUCTURED:
    case D3D_SIT_UAV_RWBYTEADDRESS:
    case D3D_SIT_UAV_APPEND_STRUCTURED:
    case D3D_SIT_UAV_CONSUME_STRUCTURED:
    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
        return ShaderDescriptorRangeType::kUAV;
    default:
        assert(false && "BuildD3D12ShaderReflection: unsupported shader input type");
        return ShaderDescriptorRangeType::kSRV;
    }
}

ShaderResourceType GetResourceType(D3D_SHADER_INPUT_TYPE type)
{
    switch (type)
    {
    case D3D_SIT_SAMPLER:
        return ShaderResourceType::kSampler;
    case D3D_SIT_RTACCELERATIONSTRUCTURE:
        return ShaderResourceType::kAccelerationStructure;
    case D3D_SIT_TEXTURE:
    case D3D_SIT_UAV_RWTYPED:
        return ShaderResourceType::kImage;
    default:
        return ShaderResourceType::kBuffer;
    }
}

ImageFormat GetInputFormat(D3D_REGISTER_COMPONENT_TYPE component_type, uint8_t component_mask)
{
    if (component_type == D3D_REGISTER_COMPONENT_FLOAT32)
    {
        if (component_mask == 0x1)
            return ImageFormat::kR32_Float;
        if (component_mask == 0x3)
            return ImageFormat::kRG32_Float;
        if (component_mask == 0x7)
            return ImageFormat::kRGB32_Float;
        if (component_mask == 0xF)
            return ImageFormat::kRGBA32_Float;
    }

    if (component_type == D3D_REGISTER_COMPONENT_UINT32)
    {
        if (component_mask == 0x1)
            return ImageFormat::kR32_UInt;
        if (component_mask == 0x3)
            return ImageFormat::kRG32_UInt;
        if (component_mask == 0x7)
            return ImageFormat::kRGB32_UInt;
        if (component_mask == 0xF)
            return ImageFormat::kRGBA32_UInt;
    }

    if (component_type == D3D_REGISTER_COMPONENT_SINT32)
    {
        if (component_mask == 0x1)
            return ImageFormat::kR32_SInt;
        if (component_mask == 0x3)
            return ImageFormat::kRG32_SInt;
        if (component_mask == 0x7)
            return ImageFormat::kRGB32_SInt;
        if (component_mask == 0xF)
            return ImageFormat::kRGBA32_SInt;
    }

    return ImageFormat::kUnknown;
}

bool IsRootConstantBuffer(char const* name, char const* root_constants_name)
{
    std::string_view const buffer_name = name ? name : "";
    std::string_view const expected_name = root_constants_name ? root_constants_name : "";
    return !expected_name.empty() && buffer_name == expected_name;
}
}  // namespace

ShaderReflection BuildD3D12ShaderReflection(ID3D12ShaderReflection* reflection, char const* root_constants_name)
{
    if (!reflection)
    {
        throw std::runtime_error("BuildD3D12ShaderReflection: reflection is null");
    }

    D3D12_SHADER_DESC shader_desc = {};
    reflection->GetDesc(&shader_desc);

    ShaderReflection result;
    result.stage = GetShaderStage(static_cast<D3D12_SHADER_VERSION_TYPE>(D3D12_SHVER_GET_TYPE(shader_desc.Version)));

    uint32_t stage_mask = static_cast<uint32_t>(result.stage);

    for (uint32_t i = 0; i < shader_desc.BoundResources; ++i)
    {
        D3D12_SHADER_INPUT_BIND_DESC resource_desc = {};
        reflection->GetResourceBindingDesc(i, &resource_desc);

        ShaderBinding binding;
        binding.name = resource_desc.Name ? resource_desc.Name : "";
        binding.binding = resource_desc.BindPoint;
        binding.space = resource_desc.Space;
        binding.resource_type = GetResourceType(resource_desc.Type);
        binding.range_type = GetRangeType(resource_desc.Type);
        binding.descriptor_count = resource_desc.BindCount;
        binding.stage_mask = stage_mask;

        if (resource_desc.Type == D3D_SIT_CBUFFER && IsRootConstantBuffer(resource_desc.Name, root_constants_name))
        {
            ID3D12ShaderReflectionConstantBuffer* cb = reflection->GetConstantBufferByName(binding.name.c_str());
            assert(cb && "BuildD3D12ShaderReflection: root constant buffer was not found");

            D3D12_SHADER_BUFFER_DESC buffer_desc = {};
            cb->GetDesc(&buffer_desc);
            if (buffer_desc.Size == 0 || (buffer_desc.Size % sizeof(uint32_t)) != 0)
            {
                throw std::runtime_error("BuildD3D12ShaderReflection: root constants must be 32-bit aligned");
            }

            binding.descriptor_type = ShaderDescriptorType::kRootConstant;
            binding.num_32bit_values = buffer_desc.Size / sizeof(uint32_t);
        }
        else
        {
            binding.descriptor_type = ShaderDescriptorType::kDescriptorTable;
        }

        result.bindings.push_back(binding);
    }

    for (uint32_t i = 0; i < shader_desc.InputParameters; ++i)
    {
        D3D12_SIGNATURE_PARAMETER_DESC parameter_desc = {};
        reflection->GetInputParameterDesc(i, &parameter_desc);

        ShaderInputParameter parameter;
        parameter.semantic_name = parameter_desc.SemanticName ? parameter_desc.SemanticName : "";
        parameter.semantic_index = parameter_desc.SemanticIndex;
        parameter.format = GetInputFormat(parameter_desc.ComponentType, parameter_desc.Mask);
        parameter.is_system_value = parameter_desc.SystemValueType != D3D_NAME_UNDEFINED;

        result.input_parameters.push_back(parameter);
    }

    return result;
}

}  // namespace gpu
