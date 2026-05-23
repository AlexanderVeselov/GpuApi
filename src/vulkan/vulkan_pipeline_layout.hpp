#pragma once

#include "../common/shader_reflection.hpp"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace gpu
{
class VulkanDevice;

struct VulkanBinding
{
    std::string name;
    uint32_t binding = 0;
    uint32_t set = 0;
    uint32_t descriptor_count = 1;
    ShaderResourceType resource_type = ShaderResourceType::kBuffer;
    ShaderDescriptorType descriptor_type = ShaderDescriptorType::kDescriptorTable;
    ShaderDescriptorRangeType range_type = ShaderDescriptorRangeType::kCBV;
    VkDescriptorType vk_descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    VkShaderStageFlags stage_flags = 0;
    uint32_t push_constant_offset = 0;
    uint32_t push_constant_size = 0;
};

class VulkanPipelineLayout
{
public:
    explicit VulkanPipelineLayout(VulkanDevice& device);
    VulkanPipelineLayout(VulkanDevice& device, std::vector<ShaderReflection const*> const& shaders);
    ~VulkanPipelineLayout();

    void Build(std::vector<ShaderReflection const*> const& shaders);
    void Clear();

    VkPipelineLayout GetPipelineLayout() const { return pipeline_layout_; }
    std::vector<VkDescriptorSetLayout> const& GetDescriptorSetLayouts() const { return descriptor_set_layouts_; }

    std::vector<VulkanBinding> const& GetBindings() const { return bindings_; }

    bool IsCompatibleWith(VulkanPipelineLayout const& other) const;

    bool HasBinding(uint32_t binding, uint32_t set) const;
    VulkanBinding const& FindBinding(uint32_t binding, uint32_t set) const;
    VulkanBinding const& FindRootConstants() const;

private:
    void AddShaderReflection(ShaderReflection const& reflection);
    void AddOrMergeBinding(VulkanBinding const& binding);
    void SortBindings();
    void CreateDescriptorSetLayouts();
    void CreatePipelineLayout();

private:
    VulkanDevice& device_;
    std::vector<VulkanBinding> bindings_;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts_;
    std::vector<VkPushConstantRange> push_constant_ranges_;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
};

}  // namespace gpu
