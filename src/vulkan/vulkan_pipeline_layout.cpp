#include "vulkan_pipeline_layout.hpp"

#include "vulkan_device.hpp"
#include "vulkan_exception.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace gpu
{
namespace
{
VkShaderStageFlags ToVkShaderStageFlags(uint32_t stage_mask)
{
    VkShaderStageFlags flags = 0;

    if ((stage_mask & static_cast<uint32_t>(ShaderStage::kVertex)) != 0)
    {
        flags |= VK_SHADER_STAGE_VERTEX_BIT;
    }

    if ((stage_mask & static_cast<uint32_t>(ShaderStage::kPixel)) != 0)
    {
        flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    if ((stage_mask & static_cast<uint32_t>(ShaderStage::kGeometry)) != 0)
    {
        flags |= VK_SHADER_STAGE_GEOMETRY_BIT;
    }

    if ((stage_mask & static_cast<uint32_t>(ShaderStage::kHull)) != 0)
    {
        flags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    }

    if ((stage_mask & static_cast<uint32_t>(ShaderStage::kDomain)) != 0)
    {
        flags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    }

    if ((stage_mask & static_cast<uint32_t>(ShaderStage::kCompute)) != 0)
    {
        flags |= VK_SHADER_STAGE_COMPUTE_BIT;
    }

    return flags;
}

VkDescriptorType ToVkDescriptorType(
    ShaderResourceType resource_type, ShaderDescriptorRangeType range_type)
{
    if (resource_type == ShaderResourceType::kBuffer)
    {
        switch (range_type)
        {
        case ShaderDescriptorRangeType::kCBV:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case ShaderDescriptorRangeType::kSRV:
        case ShaderDescriptorRangeType::kUAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        default:
            assert(false && "ToVkDescriptorType: unsupported buffer descriptor range type");
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
    }

    if (resource_type == ShaderResourceType::kSampler)
    {
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    }

    switch (range_type)
    {
    case ShaderDescriptorRangeType::kSRV:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case ShaderDescriptorRangeType::kUAV:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case ShaderDescriptorRangeType::kSampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    default:
        assert(false && "ToVkDescriptorType: unsupported descriptor range type");
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    }
}

std::string BindingName(VulkanBinding const& binding)
{
    return "(binding = " + std::to_string(binding.binding) +
           ", set = " + std::to_string(binding.set) + ")";
}
} // namespace

VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice &device) : device_(device)
{
}

VulkanPipelineLayout::VulkanPipelineLayout(
    VulkanDevice &device, std::vector<ShaderReflection const *> const& shaders)
    : device_(device)
{
    Build(shaders);
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
    Clear();
}

void VulkanPipelineLayout::Build(std::vector<ShaderReflection const *> const& shaders)
{
    Clear();

    for (ShaderReflection const *shader : shaders)
    {
        assert(shader && "VulkanPipelineLayout::Build: shader reflection pointer is null");
        AddShaderReflection(*shader);
    }

    CreateDescriptorSetLayouts();
    CreatePipelineLayout();
}

void VulkanPipelineLayout::Clear()
{
    VkDevice device = device_.GetDevice();

    if (pipeline_layout_ != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
        pipeline_layout_ = VK_NULL_HANDLE;
    }

    for (VkDescriptorSetLayout layout : descriptor_set_layouts_)
    {
        if (layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, layout, nullptr);
        }
    }

    descriptor_set_layouts_.clear();
    push_constant_ranges_.clear();
    bindings_.clear();
}

bool VulkanPipelineLayout::HasBinding(uint32_t binding, uint32_t set) const
{
    for (VulkanBinding const& vulkan_binding : bindings_)
    {
        if (vulkan_binding.binding == binding && vulkan_binding.set == set)
        {
            return true;
        }
    }

    return false;
}

VulkanBinding const& VulkanPipelineLayout::FindBinding(uint32_t binding, uint32_t set) const
{
    for (VulkanBinding const& vulkan_binding : bindings_)
    {
        if (vulkan_binding.binding == binding && vulkan_binding.set == set)
        {
            return vulkan_binding;
        }
    }

    throw std::runtime_error(
        "VulkanPipelineLayout::FindBinding: binding was not found (binding = " +
        std::to_string(binding) + ", set = " + std::to_string(set) + ")");
}

void VulkanPipelineLayout::AddShaderReflection(ShaderReflection const& reflection)
{
    for (ShaderBinding const& shader_binding : reflection.bindings)
    {
        VulkanBinding binding;
        binding.name = shader_binding.name;
        binding.binding = shader_binding.binding;
        binding.set = shader_binding.space;
        binding.descriptor_count = shader_binding.descriptor_count;
        binding.resource_type = shader_binding.resource_type;
        binding.descriptor_type = shader_binding.descriptor_type;
        binding.range_type = shader_binding.range_type;
        binding.vk_descriptor_type =
            ToVkDescriptorType(shader_binding.resource_type, shader_binding.range_type);
        binding.stage_flags = ToVkShaderStageFlags(shader_binding.stage_mask);

        if (binding.descriptor_type == ShaderDescriptorType::kRootConstant)
        {
            binding.push_constant_size = shader_binding.num_32bit_values * sizeof(uint32_t);
        }

        AddOrMergeBinding(binding);
    }
}

void VulkanPipelineLayout::AddOrMergeBinding(VulkanBinding const& binding)
{
    for (VulkanBinding &existing : bindings_)
    {
        if (existing.binding != binding.binding || existing.set != binding.set)
        {
            continue;
        }

        if (existing.resource_type != binding.resource_type ||
            existing.descriptor_type != binding.descriptor_type ||
            existing.range_type != binding.range_type ||
            existing.vk_descriptor_type != binding.vk_descriptor_type ||
            existing.descriptor_count != binding.descriptor_count ||
            existing.push_constant_size != binding.push_constant_size)
        {
            throw std::runtime_error(
                "VulkanPipelineLayout: incompatible shader bindings for " + BindingName(binding));
        }

        existing.stage_flags |= binding.stage_flags;
        return;
    }

    bindings_.push_back(binding);
}

void VulkanPipelineLayout::CreateDescriptorSetLayouts()
{
    uint32_t max_set = 0;
    bool has_descriptor_table = false;
    for (VulkanBinding const& binding : bindings_)
    {
        if (binding.descriptor_type == ShaderDescriptorType::kDescriptorTable)
        {
            if (binding.set > max_set)
            {
                max_set = binding.set;
            }
            has_descriptor_table = true;
        }
    }

    if (!has_descriptor_table)
    {
        return;
    }

    descriptor_set_layouts_.resize(max_set + 1, VK_NULL_HANDLE);

    for (uint32_t set = 0; set <= max_set; ++set)
    {
        std::vector<VkDescriptorSetLayoutBinding> set_bindings;

        for (VulkanBinding const& binding : bindings_)
        {
            if (binding.set != set ||
                binding.descriptor_type != ShaderDescriptorType::kDescriptorTable)
            {
                continue;
            }

            VkDescriptorSetLayoutBinding layout_binding = {};
            layout_binding.binding = binding.binding;
            layout_binding.descriptorType = binding.vk_descriptor_type;
            layout_binding.descriptorCount = binding.descriptor_count;
            layout_binding.stageFlags = binding.stage_flags;
            layout_binding.pImmutableSamplers = nullptr;
            set_bindings.push_back(layout_binding);
        }

        VkDescriptorSetLayoutCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.bindingCount = static_cast<uint32_t>(set_bindings.size());
        create_info.pBindings = set_bindings.empty() ? nullptr : set_bindings.data();

        VkResult status = vkCreateDescriptorSetLayout(
            device_.GetDevice(), &create_info, nullptr, &descriptor_set_layouts_[set]);
        VK_THROW_IF_FAILED(status, "Failed to create Vulkan descriptor set layout");
    }
}

void VulkanPipelineLayout::CreatePipelineLayout()
{
    uint32_t push_constant_offset = 0;
    for (VulkanBinding &binding : bindings_)
    {
        if (binding.descriptor_type != ShaderDescriptorType::kRootConstant)
        {
            continue;
        }

        binding.push_constant_offset = push_constant_offset;

        VkPushConstantRange range = {};
        range.stageFlags = binding.stage_flags;
        range.offset = binding.push_constant_offset;
        range.size = binding.push_constant_size;
        push_constant_ranges_.push_back(range);

        push_constant_offset += binding.push_constant_size;
    }

    VkPipelineLayoutCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    create_info.setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts_.size());
    create_info.pSetLayouts =
        descriptor_set_layouts_.empty() ? nullptr : descriptor_set_layouts_.data();
    create_info.pushConstantRangeCount = static_cast<uint32_t>(push_constant_ranges_.size());
    create_info.pPushConstantRanges =
        push_constant_ranges_.empty() ? nullptr : push_constant_ranges_.data();

    VkResult status =
        vkCreatePipelineLayout(device_.GetDevice(), &create_info, nullptr, &pipeline_layout_);
    VK_THROW_IF_FAILED(status, "Failed to create Vulkan pipeline layout");
}

} // namespace gpu
