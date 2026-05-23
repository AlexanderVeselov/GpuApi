#include "vulkan_shader_reflection.hpp"

#include "spirv_reflect.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace gpu
{
namespace
{
void ThrowIfSpvReflectFailed(SpvReflectResult result, char const* message)
{
    if (result != SPV_REFLECT_RESULT_SUCCESS)
    {
        throw std::runtime_error(message);
    }
}

ShaderStage ToShaderStage(SpvReflectShaderStageFlagBits stage)
{
    switch (stage)
    {
    case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT:
        return ShaderStage::kVertex;
    case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT:
        return ShaderStage::kPixel;
    case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT:
        return ShaderStage::kGeometry;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return ShaderStage::kHull;
    case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return ShaderStage::kDomain;
    case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT:
        return ShaderStage::kCompute;
    default:
        throw std::runtime_error("Unsupported SPIR-V shader stage");
    }
}

ShaderDescriptorRangeType ToRangeType(SpvReflectDescriptorType type)
{
    switch (type)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        return ShaderDescriptorRangeType::kCBV;
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return ShaderDescriptorRangeType::kSampler;
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
        return ShaderDescriptorRangeType::kUAV;
    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
    default:
        return ShaderDescriptorRangeType::kSRV;
    }
}

ShaderResourceType ToResourceType(SpvReflectDescriptorType type)
{
    switch (type)
    {
    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        return ShaderResourceType::kSampler;
    case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
        return ShaderResourceType::kAccelerationStructure;
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
        return ShaderResourceType::kBuffer;
    default:
        return ShaderResourceType::kImage;
    }
}

ImageFormat ToImageFormat(SpvReflectFormat format)
{
    switch (format)
    {
    case SPV_REFLECT_FORMAT_R32_SFLOAT:
        return ImageFormat::kR32_Float;
    case SPV_REFLECT_FORMAT_R32G32_SFLOAT:
        return ImageFormat::kRG32_Float;
    case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:
        return ImageFormat::kRGB32_Float;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:
        return ImageFormat::kRGBA32_Float;
    case SPV_REFLECT_FORMAT_R32_UINT:
        return ImageFormat::kR32_UInt;
    case SPV_REFLECT_FORMAT_R32G32_UINT:
        return ImageFormat::kRG32_UInt;
    case SPV_REFLECT_FORMAT_R32G32B32_UINT:
        return ImageFormat::kRGB32_UInt;
    case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:
        return ImageFormat::kRGBA32_UInt;
    case SPV_REFLECT_FORMAT_R32_SINT:
        return ImageFormat::kR32_SInt;
    case SPV_REFLECT_FORMAT_R32G32_SINT:
        return ImageFormat::kRG32_SInt;
    case SPV_REFLECT_FORMAT_R32G32B32_SINT:
        return ImageFormat::kRGB32_SInt;
    case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:
        return ImageFormat::kRGBA32_SInt;
    default:
        return ImageFormat::kUnknown;
    }
}

uint32_t GetDescriptorCount(SpvReflectDescriptorBinding const& binding)
{
    uint32_t array_count = 1;
    for (uint32_t i = 0; i < binding.array.dims_count; ++i)
    {
        array_count *= binding.array.dims[i];
    }

    return std::max(binding.count, array_count);
}
}  // namespace

ShaderReflection BuildVulkanShaderReflection(std::vector<uint32_t> const& spirv, char const* root_constants_name)
{
    if (!root_constants_name || root_constants_name[0] == '\0')
    {
        throw std::runtime_error("BuildVulkanShaderReflection: root constants name must not be empty");
    }

    SpvReflectShaderModule module = {};
    ThrowIfSpvReflectFailed(spvReflectCreateShaderModule(spirv.size() * sizeof(uint32_t), spirv.data(), &module),
        "BuildVulkanShaderReflection: failed to create SPIR-V reflection module");

    ShaderReflection reflection;
    reflection.stage = ToShaderStage(module.shader_stage);
    uint32_t stage_mask = static_cast<uint32_t>(reflection.stage);

    uint32_t binding_count = 0;
    ThrowIfSpvReflectFailed(spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr),
        "BuildVulkanShaderReflection: failed to enumerate descriptor binding count");

    std::vector<SpvReflectDescriptorBinding*> bindings(binding_count);
    ThrowIfSpvReflectFailed(spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings.data()),
        "BuildVulkanShaderReflection: failed to enumerate descriptor bindings");

    for (SpvReflectDescriptorBinding const* spv_binding : bindings)
    {
        ShaderBinding binding;
        binding.name = spv_binding->name ? spv_binding->name : "";
        binding.binding = spv_binding->binding;
        binding.space = spv_binding->set;
        binding.resource_type = ToResourceType(spv_binding->descriptor_type);
        binding.descriptor_type = ShaderDescriptorType::kDescriptorTable;
        binding.range_type = ToRangeType(spv_binding->descriptor_type);
        binding.descriptor_count = GetDescriptorCount(*spv_binding);
        binding.stage_mask = stage_mask;
        reflection.bindings.push_back(binding);
    }

    uint32_t push_constant_count = 0;
    ThrowIfSpvReflectFailed(spvReflectEnumeratePushConstantBlocks(&module, &push_constant_count, nullptr),
        "BuildVulkanShaderReflection: failed to enumerate push constant count");
    if (push_constant_count > 1)
    {
        spvReflectDestroyShaderModule(&module);
        throw std::runtime_error("BuildVulkanShaderReflection: multiple push-constant blocks are not supported");
    }

    if (push_constant_count == 1)
    {
        SpvReflectBlockVariable* push_constant = nullptr;
        ThrowIfSpvReflectFailed(spvReflectEnumeratePushConstantBlocks(&module, &push_constant_count, &push_constant),
            "BuildVulkanShaderReflection: failed to enumerate push constant block");

        std::string_view const push_constant_name = push_constant->name ? push_constant->name : "";
        if (push_constant_name != root_constants_name)
        {
            std::string error = "BuildVulkanShaderReflection: push constant block name '";
            error += push_constant_name;
            error += "' does not match expected root constants name '";
            error += root_constants_name;
            error += "'";
            spvReflectDestroyShaderModule(&module);
            throw std::runtime_error(error);
        }

        if (push_constant->size == 0 || (push_constant->size % sizeof(uint32_t)) != 0)
        {
            spvReflectDestroyShaderModule(&module);
            throw std::runtime_error("BuildVulkanShaderReflection: push constant block size must be 32-bit aligned");
        }

        ShaderBinding binding;
        binding.name = root_constants_name;
        binding.resource_type = ShaderResourceType::kBuffer;
        binding.descriptor_type = ShaderDescriptorType::kRootConstant;
        binding.range_type = ShaderDescriptorRangeType::kCBV;
        binding.descriptor_count = 1;
        binding.num_32bit_values = push_constant->size / sizeof(uint32_t);
        binding.stage_mask = stage_mask;
        reflection.bindings.push_back(binding);
    }

    uint32_t input_count = 0;
    ThrowIfSpvReflectFailed(spvReflectEnumerateInputVariables(&module, &input_count, nullptr),
        "BuildVulkanShaderReflection: failed to enumerate input variable count");

    std::vector<SpvReflectInterfaceVariable*> inputs(input_count);
    ThrowIfSpvReflectFailed(spvReflectEnumerateInputVariables(&module, &input_count, inputs.data()),
        "BuildVulkanShaderReflection: failed to enumerate input variables");

    for (SpvReflectInterfaceVariable const* spv_input : inputs)
    {
        if (spv_input->built_in != -1)
        {
            continue;
        }

        ShaderInputParameter input;
        input.semantic_name = spv_input->semantic ? spv_input->semantic : (spv_input->name ? spv_input->name : "");
        input.semantic_index = spv_input->location;
        input.format = ToImageFormat(spv_input->format);
        input.is_system_value = false;
        reflection.input_parameters.push_back(input);
    }

    spvReflectDestroyShaderModule(&module);
    return reflection;
}

}  // namespace gpu
